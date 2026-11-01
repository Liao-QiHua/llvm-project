//===-- RV32I.h - Top-level interface for the RV32I target ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RV32I_RV32I_H
#define LLVM_LIB_TARGET_RV32I_RV32I_H

#include "MCTargetDesc/RV32IMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class RV32ITargetMachine;
class FunctionPass;
class PassRegistry;

void initializeRV32IDAGToDAGISelPass(PassRegistry &);

FunctionPass *createRV32IISelDag(RV32ITargetMachine &TM,
                                 CodeGenOptLevel OptLevel);
} // namespace llvm

#endif // LLVM_LIB_TARGET_RV32I_RV32I_H
