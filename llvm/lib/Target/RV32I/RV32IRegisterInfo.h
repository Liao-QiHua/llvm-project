//===-- RV32IRegisterInfo.h - RV32I Register Information --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RV32I_RV32IREGISTERINFO_H
#define LLVM_LIB_TARGET_RV32I_RV32IREGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "RV32IGenRegisterInfo.inc"

namespace llvm {

struct RV32IRegisterInfo : public RV32IGenRegisterInfo {
  RV32IRegisterInfo();

  const MCPhysReg *
  getCalleeSavedRegs(const MachineFunction *MF = nullptr) const override;

  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID CC) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;

  bool eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_RV32I_RV32IREGISTERINFO_H
