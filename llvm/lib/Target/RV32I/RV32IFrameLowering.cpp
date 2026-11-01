//===-- RV32IFrameLowering.cpp - Frame lowering for RV32I -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RV32IFrameLowering.h"
#include "RV32ISubtarget.h"

using namespace llvm;

// Prologue/epilogue emission and stack frame layout are added in a later
// patch, when frame indices and spills start to appear.
