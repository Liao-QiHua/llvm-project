//===-- RV32IFrameLowering.cpp - Frame lowering for RV32I -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RV32IFrameLowering.h"
#include "RV32IInstrInfo.h"
#include "RV32ISubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

// Stack frame layout (no frame pointer; everything is sp-relative):
//
//      incoming sp  ->  [ caller frame              ]
//                       [ arguments passed on stack ]
//      new sp       ->  [ locals / spill slots      ]
//                       [ ra                        ]
//                       [ other callee-saved regs   ]
//
//  - prologue: sp -= alignTo(StackSize, 16)
//  - epilogue: sp += alignTo(StackSize, 16)
//
// The callee-saved spills/restores are emitted by the prologue/epilogue
// inserter through RV32IInstrInfo::storeRegToStackSlot/loadRegFromStackSlot.
// Frame indices are resolved relative to sp by
// RV32IRegisterInfo::eliminateFrameIndex.

void RV32IFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                              BitVector &SavedRegs,
                                              RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);

  // ra is clobbered by calls, and it is allocatable in this backend, so always
  // save it.
  SavedRegs.set(RV32I::X1);
}

void RV32IFrameLowering::emitPrologue(MachineFunction &MF,
                                      MachineBasicBlock &MBB) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const RV32IInstrInfo &TII = *STI.getInstrInfo();

  // Keep the stack 16-byte aligned (the ILP32 ABI requirement).  Reserving a
  // little more than MFI.getStackSize() is harmless: all frame objects are
  // addressed relative to the new sp.
  const uint64_t StackSize = alignTo(MFI.getStackSize(), Align(16));
  if (StackSize == 0)
    return;

  // Reserve the frame.  The callee-saved spills are inserted right after this
  // by the prologue/epilogue inserter, using sp-relative offsets.
  BuildMI(MBB, MBB.begin(), DebugLoc(), TII.get(RV32I::ADDI), RV32I::X2)
      .addReg(RV32I::X2)
      .addImm(-static_cast<int64_t>(StackSize));
}

void RV32IFrameLowering::emitEpilogue(MachineFunction &MF,
                                      MachineBasicBlock &MBB) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const RV32IInstrInfo &TII = *STI.getInstrInfo();

  const uint64_t StackSize = alignTo(MFI.getStackSize(), Align(16));
  if (StackSize == 0)
    return;

  // Release the frame.  The callee-saved restores were inserted before the
  // terminator by the prologue/epilogue inserter.
  BuildMI(MBB, MBB.getFirstTerminator(), DebugLoc(),
          TII.get(RV32I::ADDI), RV32I::X2)
      .addReg(RV32I::X2)
      .addImm(static_cast<int64_t>(StackSize));
}
