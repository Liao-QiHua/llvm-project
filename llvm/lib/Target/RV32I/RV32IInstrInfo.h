//===-- RV32IInstrInfo.h - RV32I Instruction Information --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RV32I_RV32IINSTRINFO_H
#define LLVM_LIB_TARGET_RV32I_RV32IINSTRINFO_H

#include "MCTargetDesc/RV32IMCTargetDesc.h"
#include "RV32IRegisterInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "RV32IGenInstrInfo.inc"

namespace llvm {

class RV32ISubtarget;

class RV32IInstrInfo : public RV32IGenInstrInfo {
public:
  explicit RV32IInstrInfo(const RV32ISubtarget &STI);

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
                   const DebugLoc &DL, MCRegister DestinationRegister,
                   MCRegister SourceRegister,
                   bool KillSource) const override;

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator Position,
                           Register SourceRegister, bool IsKill,
                           int FrameIndex,
                           const TargetRegisterClass *RegisterClass,
                           const TargetRegisterInfo *RegisterInfo,
                           Register VReg) const override;

  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator Position,
                            Register DestinationRegister, int FrameIndex,
                            const TargetRegisterClass *RegisterClass,
                            const TargetRegisterInfo *RegisterInfo,
                            Register VReg) const override;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_RV32I_RV32IINSTRINFO_H
