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
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc & /*DL*/,
    SelectionDAG & /*DAG*/, SmallVectorImpl<SDValue> & /*InVals*/) const {
  if (IsVarArg)
    report_fatal_error("RV32I: varargs are not supported yet");
  if (!Ins.empty())
    report_fatal_error("RV32I: arguments are implemented in a later patch");
  return Chain;
}

SDValue RV32ITargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID /*CallConv*/, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> & /*OutVals*/, const SDLoc &DL,
    SelectionDAG &DAG) const {
  if (IsVarArg)
    report_fatal_error("RV32I: varargs are not supported yet");
  if (!Outs.empty())
    report_fatal_error("RV32I: return values are implemented in a later patch");

  return DAG.getNode(RV32IISD::RET, DL, MVT::Other, Chain);
}
