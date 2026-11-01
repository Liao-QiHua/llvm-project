//===-- RV32IRegisterInfo.td - RV32I Register defs ----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RV32I has 32 general purpose 32-bit registers x0-x31.  x0 is hard-wired to
// zero.  The ABI names (zero, ra, sp, ...) are provided as alternative names
// so that the assembler/printer can use them.
//
//===----------------------------------------------------------------------===//

let Namespace = "RV32I" in {
class RV32IReg<bits<5> Enc, string n, list<string> alt = []>
    : Register<n, alt> {
  let HWEncoding{4-0} = Enc;
}
} // Namespace = "RV32I"

  let isConstant = true in
  def X0  : RV32IReg<0,  "x0",  ["zero"]>, DwarfRegNum<[0]>;
  def X1  : RV32IReg<1,  "x1",  ["ra"]>,   DwarfRegNum<[1]>;
  def X2  : RV32IReg<2,  "x2",  ["sp"]>,   DwarfRegNum<[2]>;
  def X3  : RV32IReg<3,  "x3",  ["gp"]>,   DwarfRegNum<[3]>;
  def X4  : RV32IReg<4,  "x4",  ["tp"]>,   DwarfRegNum<[4]>;
  def X5  : RV32IReg<5,  "x5",  ["t0"]>,   DwarfRegNum<[5]>;
  def X6  : RV32IReg<6,  "x6",  ["t1"]>,   DwarfRegNum<[6]>;
  def X7  : RV32IReg<7,  "x7",  ["t2"]>,   DwarfRegNum<[7]>;
  def X8  : RV32IReg<8,  "x8",  ["s0", "fp"]>, DwarfRegNum<[8]>;
  def X9  : RV32IReg<9,  "x9",  ["s1"]>,   DwarfRegNum<[9]>;
  def X10 : RV32IReg<10, "x10", ["a0"]>,   DwarfRegNum<[10]>;
  def X11 : RV32IReg<11, "x11", ["a1"]>,   DwarfRegNum<[11]>;
  def X12 : RV32IReg<12, "x12", ["a2"]>,   DwarfRegNum<[12]>;
  def X13 : RV32IReg<13, "x13", ["a3"]>,   DwarfRegNum<[13]>;
  def X14 : RV32IReg<14, "x14", ["a4"]>,   DwarfRegNum<[14]>;
  def X15 : RV32IReg<15, "x15", ["a5"]>,   DwarfRegNum<[15]>;
  def X16 : RV32IReg<16, "x16", ["a6"]>,   DwarfRegNum<[16]>;
  def X17 : RV32IReg<17, "x17", ["a7"]>,   DwarfRegNum<[17]>;
  def X18 : RV32IReg<18, "x18", ["s2"]>,   DwarfRegNum<[18]>;
  def X19 : RV32IReg<19, "x19", ["s3"]>,   DwarfRegNum<[19]>;
  def X20 : RV32IReg<20, "x20", ["s4"]>,   DwarfRegNum<[20]>;
  def X21 : RV32IReg<21, "x21", ["s5"]>,   DwarfRegNum<[21]>;
  def X22 : RV32IReg<22, "x22", ["s6"]>,   DwarfRegNum<[22]>;
  def X23 : RV32IReg<23, "x23", ["s7"]>,   DwarfRegNum<[23]>;
  def X24 : RV32IReg<24, "x24", ["s8"]>,   DwarfRegNum<[24]>;
  def X25 : RV32IReg<25, "x25", ["s9"]>,   DwarfRegNum<[25]>;
  def X26 : RV32IReg<26, "x26", ["s10"]>,  DwarfRegNum<[26]>;
  def X27 : RV32IReg<27, "x27", ["s11"]>,  DwarfRegNum<[27]>;
  def X28 : RV32IReg<28, "x28", ["t3"]>,   DwarfRegNum<[28]>;
  def X29 : RV32IReg<29, "x29", ["t4"]>,   DwarfRegNum<[29]>;
  def X30 : RV32IReg<30, "x30", ["t5"]>,   DwarfRegNum<[30]>;
  def X31 : RV32IReg<31, "x31", ["t6"]>,   DwarfRegNum<[31]>;

// The order of the registers in this class is the preferred allocation order:
// caller-saved argument/temporary registers first, callee-saved registers
// later, and the special registers (which are reserved anyway) last.
def GPR : RegisterClass<"RV32I", [i32], 32, (add
    (sequence "X%u", 10, 17), // a0-a7
    (sequence "X%u", 5, 7),   // t0-t2
    (sequence "X%u", 28, 31), // t3-t6
    (sequence "X%u", 8, 9),   // s0-s1
    (sequence "X%u", 18, 27), // s2-s11
    (sequence "X%u", 0, 4)    // zero, ra, sp, gp, tp
)>;
