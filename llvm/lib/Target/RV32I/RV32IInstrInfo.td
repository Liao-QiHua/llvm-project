//===-- RV32IInstrInfo.td - RV32I Instructions --------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RV32I instruction definitions.
//
// Patch 1 only defines a single real instruction ("ret", i.e. jalr x0, 0(ra)),
// which is enough to compile an empty function.  Later patches add the RV32I
// arithmetic, branch, load/store and jump instructions together with their
// DAG patterns and encodings.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// RV32I specific DAG nodes.
//===----------------------------------------------------------------------===//

// Return with a chain operand.  Selected to the RET instruction below.
def rv32i_ret : SDNode<"RV32IISD::RET", SDTNone, [SDNPHasChain]>;

//===----------------------------------------------------------------------===//
// Instruction base class.
//===----------------------------------------------------------------------===//

class RVInst<dag outs, dag ins, string asmstr, list<dag> pattern = []>
    : Instruction {
  let Namespace = "RV32I";

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;

  // The 32-bit encoding of the instruction, filled in by subclasses.
  field bits<32> Inst;
  // Required by TableGen's decoder generator.
  field bits<32> SoftFail = 0;

  let Size = 4;
}

//===----------------------------------------------------------------------===//
// Instructions.
//===----------------------------------------------------------------------===//

// ret == jalr x0, 0(ra)
def RET : RVInst<(outs), (ins), "ret", [(rv32i_ret)]> {
  let Inst = 0x00008067;
  let isReturn = 1;
  let isBarrier = 1;
  let isTerminator = 1;
  let hasSideEffects = 0;
}
