//===-- RV32ICallingConv.td - Calling Conventions for RV32I -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The actual argument/return value assignment is implemented by hand in
// RV32IISelLowering.cpp (see LowerFormalArguments / LowerReturn).
//
// This file only describes which registers are callee-saved, which is used by
// the register allocator and by the prologue/epilogue inserter.
//
//===----------------------------------------------------------------------===//

// ra, s0-s11 are callee-saved.  sp/gp/tp are reserved and handled separately.
def CSR_ILP32 : CalleeSavedRegs<(add X1, X8, X9, (sequence "X%u", 18, 27))>;
