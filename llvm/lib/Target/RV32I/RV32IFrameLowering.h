//===-- RV32IFrameLowering.h - Frame lowering for RV32I ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RV32I_RV32IFRAMELOWERING_H
#define LLVM_LIB_TARGET_RV32I_RV32IFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class RV32ISubtarget;

class RV32IFrameLowering : public TargetFrameLowering {
protected:
  const RV32ISubtarget &STI;

public:
  explicit RV32IFrameLowering(const RV32ISubtarget &Subtarget)
      : TargetFrameLowering(StackGrowsDown,
                            /*StackAl=*/Align(16),
                            /*LAO=*/0),
        STI(Subtarget) {}

  bool hasFP(const MachineFunction & /*MF*/) const override { return false; }

  void emitPrologue(MachineFunction & /*MF*/,
                    MachineBasicBlock & /*MBB*/) const override {}

  void emitEpilogue(MachineFunction & /*MF*/,
                    MachineBasicBlock & /*MBB*/) const override {}
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_RV32I_RV32IFRAMELOWERING_H
