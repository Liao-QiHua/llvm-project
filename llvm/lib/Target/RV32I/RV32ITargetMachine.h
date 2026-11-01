//===-- RV32ITargetMachine.h - Define TargetMachine for RV32I ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RV32I_RV32ITARGETMACHINE_H
#define LLVM_LIB_TARGET_RV32I_RV32ITARGETMACHINE_H

#include "RV32ISubtarget.h"
#include "llvm/Target/TargetMachine.h"
#include <memory>
#include <optional>

namespace llvm {

class RV32ITargetMachine : public LLVMTargetMachine {
  std::unique_ptr<RV32ISubtarget> Subtarget;
  std::unique_ptr<TargetLoweringObjectFile> TLOF;

public:
  RV32ITargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                     StringRef FS, const TargetOptions &Options,
                     std::optional<Reloc::Model> RM,
                     std::optional<CodeModel::Model> CM,
                     CodeGenOptLevel OL, bool JIT);

  const RV32ISubtarget *
  getSubtargetImpl(const Function & /*F*/) const override {
    return Subtarget.get();
  }

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_RV32I_RV32ITARGETMACHINE_H
