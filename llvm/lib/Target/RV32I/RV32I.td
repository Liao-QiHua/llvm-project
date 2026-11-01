//===-- RV32I.td - Describe the RV32I Target Machine --------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This is the top-level TableGen file for the RV32I target.
//
// RV32I is a teaching backend whose instruction set is compatible with the
// RISC-V RV32I base integer ISA, but which is a completely separate LLVM
// target: it has its own triple ("rv32i"), namespace, register file
// description, instruction definitions and MC layer.
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Register file, calling convention, scheduling resources, instructions.
//===----------------------------------------------------------------------===//

include "RV32ISchedule.td"
include "RV32IRegisterInfo.td"
include "RV32ICallingConv.td"
include "RV32IInstrInfo.td"

def RV32IInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// Processors.
//===----------------------------------------------------------------------===//

def : ProcessorModel<"generic-rv32i", RV32ISchedModel, []>;

//===----------------------------------------------------------------------===//
// Assembly writer.
//===----------------------------------------------------------------------===//

def RV32IInstPrinter : AsmWriter {
  string AsmWriterClassName = "InstPrinter";
  bit isMCAsmWriter = 1;
}

//===----------------------------------------------------------------------===//
// Declare the target.
//===----------------------------------------------------------------------===//

def RV32I : Target {
  let InstructionSet = RV32IInstrInfo;
  let AssemblyWriters = [RV32IInstPrinter];
}
