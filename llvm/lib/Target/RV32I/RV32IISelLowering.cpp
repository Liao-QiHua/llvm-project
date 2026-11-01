//===-- RV32IISelLowering.cpp - RV32I DAG Lowering Implementation ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RV32IISelLowering.h"
#include "MCTargetDesc/RV32IMCTargetDesc.h"
#include "RV32ISubtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

RV32ITargetLowering::RV32ITargetLowering(const TargetMachine &TM,
                                         const RV32ISubtarget &STI)
    : TargetLowering(TM) {
  // All integer values live in the 32-bit general purpose register file.
  addRegisterClass(MVT::i32, &RV32I::GPRRegClass);

  // The stack pointer is x2.
  setStackPointerRegisterToSaveRestore(RV32I::X2);
  setBooleanContents(ZeroOrOneBooleanContent);

  // Conditional branches are lowered to the target BR_CC node, which is then
  // selected to a B-type instruction with the right condition.
  //
  // BR_CC itself is expanded into (setcc + brcond) by the legalizer; the
  // brcond is then handled by lowerBRCOND below.
  setOperationAction(ISD::BR_CC, MVT::i32, Expand);
  setOperationAction(ISD::BRCOND, MVT::Other, Custom);

  setMinFunctionAlignment(Align(4));
  setPrefFunctionAlignment(Align(4));

  computeRegisterProperties(STI.getRegisterInfo());
}

const char *RV32ITargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch ((RV32IISD::NodeType)Opcode) {
  case RV32IISD::FIRST_NUMBER:
    break;
  case RV32IISD::RET:
    return "RV32IISD::RET";
  case RV32IISD::BR_CC:
    return "RV32IISD::BR_CC";
  case RV32IISD::CALL:
    return "RV32IISD::CALL";
  }
  return nullptr;
}

SDValue RV32ITargetLowering::LowerOperation(SDValue Op,
                                            SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  case ISD::BRCOND:
    return lowerBRCOND(Op, DAG);
  default:
    llvm_unreachable("RV32I: unexpected operation in LowerOperation");
  }
}

// brcond(setcc(lhs, rhs, cc), dest)
//   ==>  rv32i_brcc lhs, rhs, cc, dest
//
// TableGen then picks BEQ/BNE/BLT/BGE/BLTU/BGEU based on the condition code.
SDValue RV32ITargetLowering::lowerBRCOND(SDValue Op,
                                         SelectionDAG &DAG) const {
  SDValue CondV = Op.getOperand(1);
  SDLoc DL(Op);

  if (CondV.getOpcode() != ISD::SETCC ||
      CondV.getOperand(0).getValueType() != MVT::i32)
    report_fatal_error("RV32I: brcond on a non-i32 setcc is not supported yet");

  SDValue LHS = CondV.getOperand(0);
  SDValue RHS = CondV.getOperand(1);
  ISD::CondCode CC = cast<CondCodeSDNode>(CondV.getOperand(2))->get();

  // Normalize the condition to EQ/NE/LT/GE/ULT/UGE by swapping the operands
  // of the "greater than" forms.  RV32I has no BGT/BLE instruction.
  switch (CC) {
  case ISD::SETGT:
    CC = ISD::SETLT;
    std::swap(LHS, RHS);
    break;
  case ISD::SETLE:
    CC = ISD::SETGE;
    std::swap(LHS, RHS);
    break;
  case ISD::SETUGT:
    CC = ISD::SETULT;
    std::swap(LHS, RHS);
    break;
  case ISD::SETULE:
    CC = ISD::SETUGE;
    std::swap(LHS, RHS);
    break;
  default:
    break;
  }

  return DAG.getNode(RV32IISD::BR_CC, DL, Op.getValueType(), Op.getOperand(0),
                     LHS, RHS, DAG.getCondCode(CC), Op.getOperand(2));
}

SDValue RV32ITargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID /*CallConv*/, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  if (IsVarArg)
    report_fatal_error("RV32I: varargs are not supported yet");

  // The ILP32 integer calling convention: the first eight integer arguments
  // are passed in a0-a7.  Stack arguments are added in a later patch.
  static const MCPhysReg ArgRegs[] = {
      RV32I::X10, RV32I::X11, RV32I::X12, RV32I::X13,
      RV32I::X14, RV32I::X15, RV32I::X16, RV32I::X17};

  MachineFunction &MF = DAG.getMachineFunction();

  unsigned ArgNo = 0;
  for (const ISD::InputArg &Arg : Ins) {
    MVT VT = Arg.VT;
    if (VT != MVT::i32)
      report_fatal_error("RV32I: only i32 arguments are supported yet");
    if (ArgNo >= 8)
      report_fatal_error(
          "RV32I: stack arguments are implemented in a later patch");

    Register VReg = MF.addLiveIn(ArgRegs[ArgNo], &RV32I::GPRRegClass);
    InVals.push_back(DAG.getCopyFromReg(Chain, DL, VReg, VT));
    ++ArgNo;
  }

  return Chain;
}

SDValue RV32ITargetLowering::LowerCall(CallLoweringInfo &CLI,
                                       SmallVectorImpl<SDValue> &InVals) const {
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  const SDLoc &DL = CLI.DL;
  SelectionDAG &DAG = CLI.DAG;
  auto &Outs = CLI.Outs;
  auto &OutVals = CLI.OutVals;
  auto &Ins = CLI.Ins;

  if (CLI.IsVarArg)
    report_fatal_error("RV32I: varargs calls are not supported yet");
  if (CLI.IsTailCall)
    report_fatal_error("RV32I: tail calls are not supported yet");
  if (Outs.size() > 8)
    report_fatal_error(
        "RV32I: stack arguments are implemented in a later patch");

  // Only direct calls are supported: turn the callee into a symbolic operand.
  EVT PtrVT = getPointerTy(DAG.getDataLayout());
  if (Callee.getOpcode() == ISD::GlobalAddress) {
    Callee = DAG.getTargetGlobalAddress(
        cast<GlobalAddressSDNode>(Callee)->getGlobal(), DL, PtrVT);
  } else if (Callee.getOpcode() == ISD::ExternalSymbol) {
    Callee = DAG.getTargetExternalSymbol(
        cast<ExternalSymbolSDNode>(Callee)->getSymbol(), PtrVT);
  } else {
    report_fatal_error("RV32I: indirect calls are not supported yet");
  }

  // Move arguments into a0-a7.
  static const MCPhysReg ArgRegs[] = {
      RV32I::X10, RV32I::X11, RV32I::X12, RV32I::X13,
      RV32I::X14, RV32I::X15, RV32I::X16, RV32I::X17};

  for (unsigned I = 0; I < Outs.size(); ++I) {
    if (Outs[I].VT != MVT::i32)
      report_fatal_error("RV32I: only i32 arguments are supported yet");
    Chain = DAG.getCopyToReg(Chain, DL, ArgRegs[I], OutVals[I]);
  }

  // Emit the call itself.
  Chain = DAG.getNode(RV32IISD::CALL, DL, MVT::Other, Callee, Chain);

  // The result (if any) is returned in a0.
  if (Ins.size() > 1)
    report_fatal_error("RV32I: multiple return values are not supported yet");
  if (!Ins.empty()) {
    SDValue V = DAG.getCopyFromReg(Chain, DL, RV32I::X10, Ins[0].VT);
    InVals.push_back(V);
    Chain = V.getValue(1);
  }

  return Chain;
}

SDValue RV32ITargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID /*CallConv*/, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
    SelectionDAG &DAG) const {
  if (IsVarArg)
    report_fatal_error("RV32I: varargs are not supported yet");

  // Patch 02: a single integer return value is returned in a0 (x10).
  if (Outs.size() > 1)
    report_fatal_error("RV32I: multiple return values are not supported yet");

  SmallVector<SDValue, 4> RetOps(1, Chain);

  if (!Outs.empty()) {
    Chain = DAG.getCopyToReg(Chain, DL, RV32I::X10, OutVals[0]);
    // Make a0 an operand of the return node so that the copy stays alive and
    // the register is marked as live-out.
    RetOps[0] = Chain;
    RetOps.push_back(DAG.getRegister(RV32I::X10, MVT::i32));
  }

  return DAG.getNode(RV32IISD::RET, DL, MVT::Other, RetOps);
}
