//===-- RV32ITargetInfo.cpp - RV32I Target Implementation -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/RV32ITargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheRV32ITarget() {
  static Target TheRV32ITarget;
  return TheRV32ITarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRV32ITargetInfo() {
  RegisterTarget<Triple::rv32i, /*HasJIT=*/false>
      X(getTheRV32ITarget(), "rv32i", "RV32I (32-bit RISC integer base)",
        "RV32I");
}
