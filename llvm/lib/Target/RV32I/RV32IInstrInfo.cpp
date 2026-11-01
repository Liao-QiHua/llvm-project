//===-- RV32IInstrInfo.cpp - RV32I Instruction Information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RV32IInstrInfo.h"
#include "RV32ISubtarget.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "RV32IGenInstrInfo.inc"

RV32IInstrInfo::RV32IInstrInfo(const RV32ISubtarget & /*STI*/)
    : RV32IGenInstrInfo(/*CFSetupOpcode=*/-1, /*CFTearDownOpcode=*/-1) {}

void RV32IInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator Position,
                                 const DebugLoc &DL,
                                 MCRegister DestinationRegister,
                                 MCRegister SourceRegister,
                                 bool KillSource) const {
  if (!RV32I::GPRRegClass.contains(DestinationRegister, SourceRegister))
    llvm_unreachable("RV32I: impossible reg-to-reg copy");

  // mv rd, rs == addi rd, rs, 0
  BuildMI(MBB, Position, DL, get(RV32I::ADDI), DestinationRegister)
      .addReg(SourceRegister, getKillRegState(KillSource))
      .addImm(0);
}

void RV32IInstrInfo::storeRegToStackSlot(
    MachineBasicBlock & /*MBB*/, MachineBasicBlock::iterator /*Position*/,
    Register /*SourceRegister*/, bool /*IsKill*/, int /*FrameIndex*/,
    const TargetRegisterClass * /*RegisterClass*/,
    const TargetRegisterInfo * /*RegisterInfo*/, Register /*VReg*/) const {
  llvm_unreachable("RV32I: stack spills are implemented in a later patch");
}

void RV32IInstrInfo::loadRegFromStackSlot(
    MachineBasicBlock & /*MBB*/, MachineBasicBlock::iterator /*Position*/,
    Register /*DestinationRegister*/, int /*FrameIndex*/,
    const TargetRegisterClass * /*RegisterClass*/,
    const TargetRegisterInfo * /*RegisterInfo*/, Register /*VReg*/) const {
  llvm_unreachable("RV32I: stack reloads are implemented in a later patch");
}
