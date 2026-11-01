# Patch 07：目标文件与重定位

## 目标

让后端不仅能输出汇编，还能输出 **ELF32 目标文件**：

```bash
./build-rv32i/bin/llc -mtriple=rv32i-unknown-elf -O2 call.ll -filetype=obj -o call.o
```

由于 RV32I 的指令编码与 RISC-V RV32I 完全一致，我们直接生成
**真正的 RISC-V ELF 目标文件**：

```
Format: elf32-littleriscv
Machine: EM_RISCV (0xF3)
```

链接器、`objdump`（带 RISC-V 支持的版本）都可以直接消费它。

## 改动

- `RV32IInstrInfo.td`
  - 给分支/跳转/调用目标操作数加 `EncoderMethod`；
  - `PseudoBR` / `PseudoCALL` 改用带 `imm20` 位域的 `RVInstJNoRd`。
- `MCTargetDesc/RV32IFixupKinds.h`
  - `FIXUP_RV32I_BRANCH/JAL/CALL/32`。
- `MCTargetDesc/RV32IMCCodeEmitter.cpp`
  - `-gen-emitter` 生成的编码器；
  - `getBranchTargetOpValue` / `getJumpTargetOpValue` / `getCallTargetOpValue`：
    记录 fixup，返回 0。
- `MCTargetDesc/RV32IAsmBackend.cpp`
  - `applyFixup`：把位移写入 B 型 / J 型的散落位域；
  - `writeNopData`：`addi x0, x0, 0`。
- `MCTargetDesc/RV32IELFObjectWriter.cpp`
  - `EM_RISCV` + `R_RISCV_*` 重定位映射。
- `MCTargetDesc/RV32IMCTargetDesc.cpp`
  - 注册 `MCCodeEmitter`、`MCAsmBackend`、ELF streamer。

## 关键点 1：为什么第一次生成的目标文件里 `call` 变成了 `jal ra, 0`

第一版把符号操作数写成：

```tablegen
def PseudoCALL : RVInst<(outs), (ins call_symbol:$func), "call", "$func"> {
  let Inst = 0x000000EF;
}
```

结果 TableGen 生成的编码器把 `PseudoCALL` 归入"没有可变位域"的快速路径：

```cpp
case RV32I::PseudoCALL:
case RV32I::RET: {
  break;          // 不调用任何操作数编码函数
}
```

于是 `$func` 从未被编码，`getCallTargetOpValue` 从未被调用，
自然也就没有 fixup / 重定位，指令永远是 `jal ra, 0`。

修复方法是让操作数真正绑定到一个 `Inst` 位域：

```tablegen
class RVInstJNoRd<bits<7> opcode, bits<5> rdVal, dag outs, dag ins,
                  string opcodestr, string argstr>
    : RVInst<outs, ins, opcodestr, argstr> {
  bits<20> imm20;
  let Inst{31}    = imm20{19};
  let Inst{30-21} = imm20{9-0};
  let Inst{20}    = imm20{10};
  let Inst{19-12} = imm20{18-11};
  let Inst{11-7}  = rdVal;
  let Inst{6-0}   = opcode;
}

def PseudoCALL
    : RVInstJNoRd<OPC_JAL.Value, /*rd=*/1, (outs), (ins call_symbol:$imm20),
                  "call", "$imm20"> { ... }
```

这样生成的编码器才会调用 `getCallTargetOpValue`，并产生 fixup。

> 通用规律：**TableGen 只为"真正参与编码的位域"调用操作数编码函数。**
> 伪指令如果希望携带符号操作数，必须给它一个可被 fixup 回填的位域。

## 关键点 2：散落位域的回填

RISC-V 的分支/跳转立即数不是连续的：

- B 型：`imm[12|10:5] rs2 rs1 funct3 imm[4:1|11] opcode`
- J 型：`imm[20|10:1|11|19:12] rd opcode`

`applyFixup` 负责把它们打散写回 4 字节指令：

```cpp
case RV32I::FIXUP_RV32I_BRANCH: {
  uint32_t Enc = ((Imm >> 12) & 0x1) << 31;
  Enc |= ((Imm >> 5) & 0x3F) << 25;
  Enc |= ((Imm >> 1) & 0xF) << 8;
  Enc |= ((Imm >> 11) & 0x1) << 7;
  ...
}
case RV32I::FIXUP_RV32I_JAL:
case RV32I::FIXUP_RV32I_CALL: {
  uint32_t Enc = ((Imm >> 20) & 0x1) << 31;
  Enc |= ((Imm >> 1) & 0x3FF) << 21;
  Enc |= ((Imm >> 11) & 0x1) << 20;
  Enc |= ((Imm >> 12) & 0xFF) << 12;
  Inst = (Inst & 0xFFFu) | Enc;   // 保留 rd + opcode
}
```

## 关键点 3：什么时候回填，什么时候留重定位

- **同一 section 的局部标签**（基本块）：汇编器可以算出位移，
  直接 `applyFixup` 回填，目标文件里没有重定位；
- **全局/未定义符号**（函数调用）：保留 `R_RISCV_CALL` 重定位，
  由链接器计算最终地址。

验证：

```bash
./build-rv32i/bin/llc -mtriple=rv32i-unknown-elf -O2 call.ll \
  -filetype=obj -o call.o
./build-rv32i/bin/llvm-readobj --relocations call.o
```

```
Relocations [
  Section (3) .rela.text {
    0x20 R_RISCV_CALL callee 0x0
    0x50 R_RISCV_CALL rec 0x0
  }
  ...
]
```

而条件分支（`blt` 等）没有重定位，因为它们的位移已经写进指令。

## 复现

```bash
ninja -C build-rv32i llc
for f in retvoid const alu cf mem call extcall; do
  ./build-rv32i/bin/llc -mtriple=rv32i-unknown-elf -O2 /tmp/$f.ll \
    -filetype=obj -o /tmp/$f.o
done
./build-rv32i/bin/llvm-readobj --file-headers --relocations /tmp/call.o
```

> 提示：本构建只包含 `RV32I` 目标，`llvm-objdump` 无法反汇编 RISC-V
> （它需要 `RISCV` 后端）。可以用 `llvm-readobj` + `xxd` 检查，
> 或者另外配置一个带 `RISCV` 目标的构建来交叉验证。

## 本补丁的边界

- 还没有全局数据寻址（`lui %hi / addi %lo`、`R_RISCV_HI20/LO12`）；
- 没有链接器松弛（relaxation）；
- 没有汇编器/反汇编器（patch 08）。
