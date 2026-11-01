//===-- RV32ITargetMachine.cpp - Define TargetMachine for RV32I -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RV32ITargetMachine.h"
#include "RV32I.h"
#include "TargetInfo/RV32ITargetInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Target/TargetOptions.h"
#include <optional>

using namespace llvm;

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRV32ITarget() {
  RegisterTargetMachine<RV32ITargetMachine> X(getTheRV32ITarget());

  PassRegistry &PR = *PassRegistry::getPassRegistry();
  initializeRV32IDAGToDAGISelPass(PR);
}

static std::string computeDataLayout() {
  // Little endian, 32-bit pointers, 32-bit integers, natural stack alignment.
  return "e-m:e-p:32:32-i64:64-n32-S128";
}

RV32ITargetMachine::RV32ITargetMachine(const Target &T, const Triple &TT,
                                       StringRef CPU, StringRef FS,
                                       const TargetOptions &Options,
                                       std::optional<Reloc::Model> RM,
                                       std::optional<CodeModel::Model> CM,
                                       CodeGenOptLevel OL, bool /*JIT*/)
    : LLVMTargetMachine(T, computeDataLayout(), TT, CPU, FS, Options,
                        RM.value_or(Reloc::Static),
                        CM.value_or(CodeModel::Small), OL),
      Subtarget(std::make_unique<RV32ISubtarget>(TT, CPU, /*TuneCPU=*/CPU, FS,
                                                 *this)),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()) {
  initAsmInfo();
}

namespace {

class RV32IPassConfig : public TargetPassConfig {
public:
  RV32IPassConfig(RV32ITargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  bool addInstSelector() override;
};

} // end anonymous namespace

bool RV32IPassConfig::addInstSelector() {
  addPass(createRV32IISelDag(getTM<RV32ITargetMachine>(), getOptLevel()));
  return false;
}

TargetPassConfig *
RV32ITargetMachine::createPassConfig(PassManagerBase &PM) {
  return new RV32IPassConfig(*this, PM);
}
