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
// Patch 04 adds control flow: conditional branches (B-type), unconditional
// jumps (J-type) and the custom BR_CC node used to select them.
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

// Conditional branch: (rv32i_brcc lhs, rhs, condcode, dest), with chain.
def SDT_RV32IBrCC : SDTypeProfile<0, 4, [SDTCisSameAs<0, 1>,
                                         SDTCisVT<2, OtherVT>,
                                         SDTCisVT<3, OtherVT>]>;
def rv32i_brcc : SDNode<"RV32IISD::BR_CC", SDT_RV32IBrCC, [SDNPHasChain]>;

// Direct call: (rv32i_call callee), with chain.
def SDT_RV32ICall : SDTypeProfile<0, 1, [SDTCisVT<0, i32>]>;
def rv32i_call : SDNode<"RV32IISD::CALL", SDT_RV32ICall, [SDNPHasChain]>;

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

// Branch/jump targets.  These are not immediates at the DAG level: instruction
// selection binds them to basic blocks (bb operands).
def simm13_lsb0 : Operand<OtherVT>;
def simm21_lsb0 : Operand<OtherVT>;
// A direct call target (symbol reference).
def call_symbol : Operand<i32>;

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

// B-type: imm[12|10:5] rs2[24:20] rs1[19:15] funct3[14:12] imm[4:1|11] opcode[6:0]
class RVInstB<bits<3> funct3, bits<7> opcode, dag outs, dag ins,
              string opcodestr, string argstr, list<dag> pattern = []>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<12> imm12;
  bits<5> rs2;
  bits<5> rs1;

  let Inst{31} = imm12{11};
  let Inst{30-25} = imm12{9-4};
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-8} = imm12{3-0};
  let Inst{7} = imm12{10};
  let Inst{6-0} = opcode;
}

// S-type: imm[11:5] rs2[24:20] rs1[19:15] funct3[14:12] imm[4:0] opcode[6:0]
class RVInstS<bits<3> funct3, bits<7> opcode, dag outs, dag ins,
              string opcodestr, string argstr, list<dag> pattern = []>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<12> imm12;
  bits<5> rs2;
  bits<5> rs1;

  let Inst{31-25} = imm12{11-5};
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = imm12{4-0};
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

// J-type: imm[20|10:1|11|19:12] rd[11:7] opcode[6:0]
class RVInstJ<bits<7> opcode, dag outs, dag ins, string opcodestr,
              string argstr, list<dag> pattern = []>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<20> imm20;
  bits<5> rd;

  let Inst{31} = imm20{19};
  let Inst{30-21} = imm20{9-0};
  let Inst{20} = imm20{10};
  let Inst{19-12} = imm20{18-11};
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

// --- control flow ---
def BEQ : RVInstB<0b000, OPC_BRANCH.Value, (outs),
                  (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
                  "beq", "$rs1, $rs2, $imm12">,
          Sched<[WriteJmp, ReadIALU, ReadIALU]> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
def BNE : RVInstB<0b001, OPC_BRANCH.Value, (outs),
                  (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
                  "bne", "$rs1, $rs2, $imm12">,
          Sched<[WriteJmp, ReadIALU, ReadIALU]> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
def BLT : RVInstB<0b100, OPC_BRANCH.Value, (outs),
                  (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
                  "blt", "$rs1, $rs2, $imm12">,
          Sched<[WriteJmp, ReadIALU, ReadIALU]> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
def BGE : RVInstB<0b101, OPC_BRANCH.Value, (outs),
                  (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
                  "bge", "$rs1, $rs2, $imm12">,
          Sched<[WriteJmp, ReadIALU, ReadIALU]> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
def BLTU : RVInstB<0b110, OPC_BRANCH.Value, (outs),
                   (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
                   "bltu", "$rs1, $rs2, $imm12">,
           Sched<[WriteJmp, ReadIALU, ReadIALU]> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}
def BGEU : RVInstB<0b111, OPC_BRANCH.Value, (outs),
                   (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
                   "bgeu", "$rs1, $rs2, $imm12">,
           Sched<[WriteJmp, ReadIALU, ReadIALU]> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

// jal x0, target == j target  (unconditional branch)
def JAL : RVInstJ<OPC_JAL.Value, (outs GPR:$rd), (ins simm21_lsb0:$imm20),
                  "jal", "$rd, $imm20">, Sched<[WriteJal]> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

// Direct function call.  ra (x1) is implicitly defined.  For assembly output
// it is printed as "call symbol"; the encoding and relocation are handled by
// the MC layer in a later patch.
def PseudoCALL : RVInst<(outs), (ins call_symbol:$func), "call", "$func"> {
  let Inst = 0x000000EF; // jal ra, 0
  let isCall = 1;
  let Defs = [X1];
  let hasSideEffects = 1;
}

// Unconditional branch.  It has no destination register, so it cannot be
// written as "jal x0, target" in a pattern (patterns only provide input
// operands); it is printed as the "j" alias instead.
def PseudoBR : RVInst<(outs), (ins simm21_lsb0:$imm20), "j", "$imm20"> {
  let Inst = 0x0000006F; // jal x0, 0
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
  let isCodeGenOnly = 1;
}

// --- loads and stores are defined outside the "no side effects" block,
// because they are memory operations.

} // hasSideEffects = 0, mayLoad = 0, mayStore = 0

class LoadI<bits<3> funct3, string opcodestr>
    : RVInstI<funct3, OPC_LOAD.Value, (outs GPR:$rd),
              (ins GPR:$rs1, simm12:$imm12),
              opcodestr, "$rd, ${imm12}(${rs1})"> {
  let mayLoad = 1;
  let mayStore = 0;
}

class StoreS<bits<3> funct3, string opcodestr>
    : RVInstS<funct3, OPC_STORE.Value, (outs),
              (ins GPR:$rs2, GPR:$rs1, simm12:$imm12),
              opcodestr, "$rs2, ${imm12}(${rs1})"> {
  let mayLoad = 0;
  let mayStore = 1;
}

def LB  : LoadI<0b000, "lb">,  Sched<[WriteLoad, ReadMemBase]>;
def LH  : LoadI<0b001, "lh">,  Sched<[WriteLoad, ReadMemBase]>;
def LW  : LoadI<0b010, "lw">,  Sched<[WriteLoad, ReadMemBase]>;
def LBU : LoadI<0b100, "lbu">, Sched<[WriteLoad, ReadMemBase]>;
def LHU : LoadI<0b101, "lhu">, Sched<[WriteLoad, ReadMemBase]>;

def SB : StoreS<0b000, "sb">, Sched<[WriteStore, ReadStoreData, ReadMemBase]>;
def SH : StoreS<0b001, "sh">, Sched<[WriteStore, ReadStoreData, ReadMemBase]>;
def SW : StoreS<0b010, "sw">, Sched<[WriteStore, ReadStoreData, ReadMemBase]>;

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

// Conditional branches.  The CondCode value selects which B-type
// instruction implements the comparison.
multiclass BccPat<CondCode Cond, RVInstB Inst> {
  def : Pat<(rv32i_brcc GPR:$rs1, GPR:$rs2, Cond, bb:$dst),
            (Inst GPR:$rs1, GPR:$rs2, bb:$dst)>;
}

defm : BccPat<SETEQ, BEQ>;
defm : BccPat<SETNE, BNE>;
defm : BccPat<SETLT, BLT>;
defm : BccPat<SETGE, BGE>;
defm : BccPat<SETULT, BLTU>;
defm : BccPat<SETUGE, BGEU>;

// Unconditional branch: j dest
def : Pat<(br bb:$dst), (PseudoBR bb:$dst)>;
