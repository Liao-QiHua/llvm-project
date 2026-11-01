//===-- RV32IRegisterInfo.cpp - RV32I Register Information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RV32IRegisterInfo.h"
#include "MCTargetDesc/RV32IMCTargetDesc.h"
#include "RV32ISubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "RV32IGenRegisterInfo.inc"

RV32IRegisterInfo::RV32IRegisterInfo() : RV32IGenRegisterInfo(RV32I::X1) {}

const MCPhysReg *
RV32IRegisterInfo::getCalleeSavedRegs(const MachineFunction * /*MF*/) const {
  return CSR_ILP32_SaveList;
}

const uint32_t *
RV32IRegisterInfo::getCallPreservedMask(const MachineFunction & /*MF*/,
                                        CallingConv::ID /*CC*/) const {
  return CSR_ILP32_RegMask;
}

BitVector RV32IRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  // x0 is hard-wired to zero; sp/gp/tp are reserved by the ABI.
  markSuperRegs(Reserved, RV32I::X0);
  markSuperRegs(Reserved, RV32I::X2);
  markSuperRegs(Reserved, RV32I::X3);
  markSuperRegs(Reserved, RV32I::X4);

  return Reserved;
}

bool RV32IRegisterInfo::eliminateFrameIndex(
    MachineBasicBlock::iterator /*II*/, int /*SPAdj*/, unsigned /*FIOperandNum*/,
    RegScavenger * /*RS*/) const {
  // Stack frame support is added in a later patch.
  llvm_unreachable("RV32I: frame index elimination is implemented later");
}

Register RV32IRegisterInfo::getFrameRegister(const MachineFunction & /*MF*/) const {
  return RV32I::X8;
}
