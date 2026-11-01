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
  }
  return nullptr;
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
