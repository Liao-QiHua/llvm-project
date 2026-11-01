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
    MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
    Register SourceRegister, bool IsKill, int FrameIndex,
    const TargetRegisterClass *RegisterClass,
    const TargetRegisterInfo * /*RegisterInfo*/, Register /*VReg*/) const {
  if (!RV32I::GPRRegClass.hasSubClassEq(RegisterClass))
    llvm_unreachable("RV32I: can only spill GPRs");

  BuildMI(MBB, Position, DebugLoc(), get(RV32I::SW))
      .addReg(SourceRegister, getKillRegState(IsKill))
      .addFrameIndex(FrameIndex)
      .addImm(0);
}

void RV32IInstrInfo::loadRegFromStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
    Register DestinationRegister, int FrameIndex,
    const TargetRegisterClass *RegisterClass,
    const TargetRegisterInfo * /*RegisterInfo*/, Register /*VReg*/) const {
  if (!RV32I::GPRRegClass.hasSubClassEq(RegisterClass))
    llvm_unreachable("RV32I: can only reload GPRs");

  BuildMI(MBB, Position, DebugLoc(), get(RV32I::LW), DestinationRegister)
      .addFrameIndex(FrameIndex)
      .addImm(0);
}

static bool isConditionalBranch(unsigned Opcode) {
  switch (Opcode) {
  case RV32I::BEQ:
  case RV32I::BNE:
  case RV32I::BLT:
  case RV32I::BGE:
  case RV32I::BLTU:
  case RV32I::BGEU:
    return true;
  default:
    return false;
  }
}

// Recognized forms:
//   [conditional branch]              -> TBB, FBB = nullptr
//   [conditional branch] ; j target   -> TBB, FBB
//   j target                          -> TBB, FBB = nullptr
// Anything else (e.g. a return) cannot be analyzed.
bool RV32IInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                   MachineBasicBlock *&TBB,
                                   MachineBasicBlock *&FBB,
                                   SmallVectorImpl<MachineOperand> &Cond,
                                   bool /*AllowModify*/) const {
  TBB = nullptr;
  FBB = nullptr;
  Cond.clear();

  if (MBB.empty())
    return false;

  MachineBasicBlock::iterator I = MBB.end();
  --I;
  while (I != MBB.begin() && I->isDebugInstr())
    --I;

  if (I->getOpcode() == RV32I::PseudoBR) {
    MachineBasicBlock *UncondDest = I->getOperand(0).getMBB();

    if (I != MBB.begin()) {
      MachineBasicBlock::iterator Prev = std::prev(I);
      while (Prev != MBB.begin() && Prev->isDebugInstr())
        --Prev;
      if (isConditionalBranch(Prev->getOpcode())) {
        Cond.push_back(MachineOperand::CreateImm(Prev->getOpcode()));
        Cond.push_back(Prev->getOperand(0));
        Cond.push_back(Prev->getOperand(1));
        TBB = Prev->getOperand(2).getMBB();
        FBB = UncondDest;
        return false;
      }
    }

    TBB = UncondDest;
    return false;
  }

  if (isConditionalBranch(I->getOpcode())) {
    Cond.push_back(MachineOperand::CreateImm(I->getOpcode()));
    Cond.push_back(I->getOperand(0));
    Cond.push_back(I->getOperand(1));
    TBB = I->getOperand(2).getMBB();
    return false;
  }

  return true;
}

unsigned RV32IInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                      int *BytesRemoved) const {
  unsigned Count = 0;
  while (!MBB.empty()) {
    MachineInstr &MI = MBB.back();
    if (!MI.isBranch())
      break;
    MI.eraseFromParent();
    ++Count;
  }

  if (BytesRemoved)
    *BytesRemoved = Count * 4;
  return Count;
}

unsigned RV32IInstrInfo::insertBranch(MachineBasicBlock &MBB,
                                      MachineBasicBlock *TBB,
                                      MachineBasicBlock *FBB,
                                      ArrayRef<MachineOperand> Cond,
                                      const DebugLoc &DL,
                                      int *BytesAdded) const {
  unsigned Bytes = 0;

  if (!Cond.empty()) {
    unsigned Opc = Cond[0].getImm();
    BuildMI(&MBB, DL, get(Opc)).add(Cond[1]).add(Cond[2]).addMBB(TBB);
    Bytes += 4;
    if (!FBB) {
      if (BytesAdded)
        *BytesAdded = Bytes;
      return 1;
    }
  }

  BuildMI(&MBB, DL, get(RV32I::PseudoBR)).addMBB(FBB ? FBB : TBB);
  Bytes += 4;

  if (BytesAdded)
    *BytesAdded = Bytes;
  return Cond.empty() ? 1 : 2;
}

bool RV32IInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  if (Cond.empty())
    return true;

  unsigned Opc = Cond[0].getImm();
  unsigned Inv;
  switch (Opc) {
  case RV32I::BEQ:
    Inv = RV32I::BNE;
    break;
  case RV32I::BNE:
    Inv = RV32I::BEQ;
    break;
  case RV32I::BLT:
    Inv = RV32I::BGE;
    break;
  case RV32I::BGE:
    Inv = RV32I::BLT;
    break;
  case RV32I::BLTU:
    Inv = RV32I::BGEU;
    break;
  case RV32I::BGEU:
    Inv = RV32I::BLTU;
    break;
  default:
    return true;
  }

  Cond[0].setImm(Inv);
  return false;
}
