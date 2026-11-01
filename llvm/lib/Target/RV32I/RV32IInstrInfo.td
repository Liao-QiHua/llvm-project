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
// Patch 03 adds the complete integer ALU subset: add/sub, and/or/xor,
// slt/sltu, shifts by immediate and by register, plus the DAG patterns that
// select them.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// RV32I specific DAG nodes.
//===----------------------------------------------------------------------===//

// Return with a chain operand.  The node is variadic so that LowerReturn can
// attach the physical return registers (e.g. a0) as extra operands; this keeps
// the CopyToReg nodes alive until instruction selection.  Selected to RET below.
def rv32i_ret : SDNode<"RV32IISD::RET", SDTNone,
                       [SDNPHasChain, SDNPVariadic]>;

//===----------------------------------------------------------------------===//
// Operands.
//===----------------------------------------------------------------------===//

// 12-bit signed immediate, also usable as a pattern leaf.
def simm12 : Operand<i32>, ImmLeaf<i32, [{ return isInt<12>(Imm); }]>;

// 20-bit unsigned immediate (LUI/AUIPC).
def uimm20 : Operand<i32>, ImmLeaf<i32, [{ return isUInt<20>(Imm); }]>;

// Shift amount for RV32: 0..31.
def uimmlog2xlen : Operand<i32>, ImmLeaf<i32, [{
  return Imm >= 0 && Imm < 32;
}]>;

//===----------------------------------------------------------------------===//
// Instruction base classes.
//===----------------------------------------------------------------------===//

class RVInst<dag outs, dag ins, string opcodestr, string argstr,
             list<dag> pattern = []> : Instruction {
  let Namespace = "RV32I";

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = opcodestr # !if(!empty(argstr), "", "\t" # argstr);
  let Pattern = pattern;

  // The 32-bit encoding of the instruction, filled in by subclasses.
  field bits<32> Inst;
  // Required by TableGen's decoder generator.
  field bits<32> SoftFail = 0;

  let Size = 4;
}

// R-type: funct7[31:25] rs2[24:20] rs1[19:15] funct3[14:12] rd[11:7] opcode[6:0]
class RVInstR<bits<7> funct7, bits<3> funct3, bits<7> opcode, dag outs, dag ins,
              string opcodestr, string argstr, list<dag> pattern = []>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<5> rs2;
  bits<5> rs1;
  bits<5> rd;

  let Inst{31-25} = funct7;
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = rd;
  let Inst{6-0} = opcode;
}

// I-type: imm[31:20] rs1[19:15] funct3[14:12] rd[11:7] opcode[6:0]
class RVInstI<bits<3> funct3, bits<7> opcode, dag outs, dag ins,
              string opcodestr, string argstr, list<dag> pattern = []>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<12> imm12;
  bits<5> rs1;
  bits<5> rd;

  let Inst{31-20} = imm12;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = rd;
  let Inst{6-0} = opcode;
}

// I-type shift: funct7[31:25] shamt[24:20] rs1[19:15] funct3[14:12] rd[11:7] opcode[6:0]
class RVInstIShift<bits<7> funct7, bits<3> funct3, bits<7> opcode, dag outs,
                   dag ins, string opcodestr, string argstr,
                   list<dag> pattern = []>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<5> shamt;
  bits<5> rs1;
  bits<5> rd;

  let Inst{31-25} = funct7;
  let Inst{24-20} = shamt;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = rd;
  let Inst{6-0} = opcode;
}

// U-type: imm[31:12] rd[11:7] opcode[6:0]
class RVInstU<bits<7> opcode, dag outs, dag ins, string opcodestr,
              string argstr, list<dag> pattern = []>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<20> imm20;
  bits<5> rd;

  let Inst{31-12} = imm20;
  let Inst{11-7} = rd;
  let Inst{6-0} = opcode;
}

//===----------------------------------------------------------------------===//
// Opcodes (RISC-V base opcode map).
//===----------------------------------------------------------------------===//

class RV32IOpcode<bits<7> val> {
  bits<7> Value = val;
}

def OPC_LOAD    : RV32IOpcode<0b0000011>;
def OPC_OP_IMM  : RV32IOpcode<0b0010011>;
def OPC_AUIPC   : RV32IOpcode<0b0010111>;
def OPC_STORE   : RV32IOpcode<0b0100011>;
def OPC_OP      : RV32IOpcode<0b0110011>;
def OPC_LUI     : RV32IOpcode<0b0110111>;
def OPC_BRANCH  : RV32IOpcode<0b1100011>;
def OPC_JALR    : RV32IOpcode<0b1100111>;
def OPC_JAL     : RV32IOpcode<0b1101111>;

//===----------------------------------------------------------------------===//
// Instructions.
//===----------------------------------------------------------------------===//

// ret == jalr x0, 0(ra)
def RET : RVInst<(outs), (ins), "ret", "", [(rv32i_ret)]> {
  let Inst = 0x00008067;
  let isReturn = 1;
  let isBarrier = 1;
  let isTerminator = 1;
  let hasSideEffects = 0;
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in {

def LUI : RVInstU<OPC_LUI.Value, (outs GPR:$rd), (ins uimm20:$imm20),
                  "lui", "$rd, $imm20">, Sched<[WriteIALU]>;

// --- add/sub ---
def ADDI : RVInstI<0b000, OPC_OP_IMM.Value, (outs GPR:$rd),
                   (ins GPR:$rs1, simm12:$imm12),
                   "addi", "$rd, $rs1, $imm12">,
           Sched<[WriteIALU, ReadIALU]>;
def ADD : RVInstR<0b0000000, 0b000, OPC_OP.Value, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "add", "$rd, $rs1, $rs2">,
          Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def SUB : RVInstR<0b0100000, 0b000, OPC_OP.Value, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "sub", "$rd, $rs1, $rs2">,
          Sched<[WriteIALU, ReadIALU, ReadIALU]>;

// --- logical ---
def ANDI : RVInstI<0b111, OPC_OP_IMM.Value, (outs GPR:$rd),
                   (ins GPR:$rs1, simm12:$imm12),
                   "andi", "$rd, $rs1, $imm12">,
           Sched<[WriteIALU, ReadIALU]>;
def ORI : RVInstI<0b110, OPC_OP_IMM.Value, (outs GPR:$rd),
                  (ins GPR:$rs1, simm12:$imm12),
                  "ori", "$rd, $rs1, $imm12">,
          Sched<[WriteIALU, ReadIALU]>;
def XORI : RVInstI<0b100, OPC_OP_IMM.Value, (outs GPR:$rd),
                   (ins GPR:$rs1, simm12:$imm12),
                   "xori", "$rd, $rs1, $imm12">,
           Sched<[WriteIALU, ReadIALU]>;
def AND : RVInstR<0b0000000, 0b111, OPC_OP.Value, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "and", "$rd, $rs1, $rs2">,
          Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def OR : RVInstR<0b0000000, 0b110, OPC_OP.Value, (outs GPR:$rd),
                 (ins GPR:$rs1, GPR:$rs2), "or", "$rd, $rs1, $rs2">,
         Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def XOR : RVInstR<0b0000000, 0b100, OPC_OP.Value, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "xor", "$rd, $rs1, $rs2">,
          Sched<[WriteIALU, ReadIALU, ReadIALU]>;

// --- set less than ---
def SLTI : RVInstI<0b010, OPC_OP_IMM.Value, (outs GPR:$rd),
                   (ins GPR:$rs1, simm12:$imm12),
                   "slti", "$rd, $rs1, $imm12">,
           Sched<[WriteIALU, ReadIALU]>;
def SLTIU : RVInstI<0b011, OPC_OP_IMM.Value, (outs GPR:$rd),
                    (ins GPR:$rs1, simm12:$imm12),
                    "sltiu", "$rd, $rs1, $imm12">,
            Sched<[WriteIALU, ReadIALU]>;
def SLT : RVInstR<0b0000000, 0b010, OPC_OP.Value, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "slt", "$rd, $rs1, $rs2">,
          Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def SLTU : RVInstR<0b0000000, 0b011, OPC_OP.Value, (outs GPR:$rd),
                   (ins GPR:$rs1, GPR:$rs2), "sltu", "$rd, $rs1, $rs2">,
           Sched<[WriteIALU, ReadIALU, ReadIALU]>;

// --- shifts ---
def SLLI : RVInstIShift<0b0000000, 0b001, OPC_OP_IMM.Value, (outs GPR:$rd),
                        (ins GPR:$rs1, uimmlog2xlen:$shamt),
                        "slli", "$rd, $rs1, $shamt">,
           Sched<[WriteShift, ReadIALU]>;
def SRLI : RVInstIShift<0b0000000, 0b101, OPC_OP_IMM.Value, (outs GPR:$rd),
                        (ins GPR:$rs1, uimmlog2xlen:$shamt),
                        "srli", "$rd, $rs1, $shamt">,
           Sched<[WriteShift, ReadIALU]>;
def SRAI : RVInstIShift<0b0100000, 0b101, OPC_OP_IMM.Value, (outs GPR:$rd),
                        (ins GPR:$rs1, uimmlog2xlen:$shamt),
                        "srai", "$rd, $rs1, $shamt">,
           Sched<[WriteShift, ReadIALU]>;
def SLL : RVInstR<0b0000000, 0b001, OPC_OP.Value, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "sll", "$rd, $rs1, $rs2">,
          Sched<[WriteShift, ReadIALU, ReadIALU]>;
def SRL : RVInstR<0b0000000, 0b101, OPC_OP.Value, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "srl", "$rd, $rs1, $rs2">,
          Sched<[WriteShift, ReadIALU, ReadIALU]>;
def SRA : RVInstR<0b0100000, 0b101, OPC_OP.Value, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "sra", "$rd, $rs1, $rs2">,
          Sched<[WriteShift, ReadIALU, ReadIALU]>;

} // hasSideEffects = 0, mayLoad = 0, mayStore = 0

//===----------------------------------------------------------------------===//
// DAG patterns.
//===----------------------------------------------------------------------===//

def : Pat<(i32 (add GPR:$rs1, GPR:$rs2)), (ADD GPR:$rs1, GPR:$rs2)>;
def : Pat<(i32 (add GPR:$rs1, simm12:$imm)), (ADDI GPR:$rs1, simm12:$imm)>;

// "sub x, C" is canonicalized by DAGCombine into "add x, -C", so only the
// register-register form needs a pattern here.
def : Pat<(i32 (sub GPR:$rs1, GPR:$rs2)), (SUB GPR:$rs1, GPR:$rs2)>;

def : Pat<(i32 (and GPR:$rs1, GPR:$rs2)), (AND GPR:$rs1, GPR:$rs2)>;
def : Pat<(i32 (and GPR:$rs1, simm12:$imm)), (ANDI GPR:$rs1, simm12:$imm)>;
def : Pat<(i32 (or GPR:$rs1, GPR:$rs2)), (OR GPR:$rs1, GPR:$rs2)>;
def : Pat<(i32 (or GPR:$rs1, simm12:$imm)), (ORI GPR:$rs1, simm12:$imm)>;
def : Pat<(i32 (xor GPR:$rs1, GPR:$rs2)), (XOR GPR:$rs1, GPR:$rs2)>;
def : Pat<(i32 (xor GPR:$rs1, simm12:$imm)), (XORI GPR:$rs1, simm12:$imm)>;

def : Pat<(i32 (setlt GPR:$rs1, GPR:$rs2)), (SLT GPR:$rs1, GPR:$rs2)>;
def : Pat<(i32 (setlt GPR:$rs1, simm12:$imm)), (SLTI GPR:$rs1, simm12:$imm)>;
def : Pat<(i32 (setult GPR:$rs1, GPR:$rs2)), (SLTU GPR:$rs1, GPR:$rs2)>;
def : Pat<(i32 (setult GPR:$rs1, simm12:$imm)), (SLTIU GPR:$rs1, simm12:$imm)>;

def : Pat<(i32 (shl GPR:$rs1, GPR:$rs2)), (SLL GPR:$rs1, GPR:$rs2)>;
def : Pat<(i32 (shl GPR:$rs1, uimmlog2xlen:$imm)),
          (SLLI GPR:$rs1, uimmlog2xlen:$imm)>;
def : Pat<(i32 (srl GPR:$rs1, GPR:$rs2)), (SRL GPR:$rs1, GPR:$rs2)>;
def : Pat<(i32 (srl GPR:$rs1, uimmlog2xlen:$imm)),
          (SRLI GPR:$rs1, uimmlog2xlen:$imm)>;
def : Pat<(i32 (sra GPR:$rs1, GPR:$rs2)), (SRA GPR:$rs1, GPR:$rs2)>;
def : Pat<(i32 (sra GPR:$rs1, uimmlog2xlen:$imm)),
          (SRAI GPR:$rs1, uimmlog2xlen:$imm)>;
