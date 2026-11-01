# Patch 04：控制流

## 目标

支持条件分支、无条件跳转、循环：

```llvm
define i32 @max(i32 %a, i32 %b) {
  %c = icmp sgt i32 %a, %b
  br i1 %c, label %t, label %f
t: ret i32 %a
f: ret i32 %b
}
```

```asm
max:
	bge a1, a0, .LBB0_2
	j   .LBB0_1
.LBB0_1:
	ret
.LBB0_2:
	addi a0, a1, 0
	ret
```

## 改动

- `RV32IInstrInfo.td`
  - B 型指令 `BEQ/BNE/BLT/BGE/BLTU/BGEU`；
  - J 型指令 `JAL`，以及无目的寄存器的 `PseudoBR`（打印为 `j`）；
  - 目标节点 `RV32IISD::BR_CC` 与 `BccPat` 多播模式；
  - `(br bb)` → `PseudoBR`。
- `RV32IISelLowering.cpp`
  - `BR_CC` 设为 `Expand`，`BRCOND` 设为 `Custom`；
  - `LowerOperation` → `lowerBRCOND`，把 `brcond(setcc)` 变成 `RV32IISD::BR_CC`；
  - 条件规范化（`SETGT/SETLE/SETUGT/SETULE` 交换操作数）。

## 关键点 1：为什么要 "Expand BR_CC + Custom BRCOND"

DAG 里的条件分支可能以两种形态出现：

1. `br_cc chain, lhs, rhs, cc, dest`（直接来自 `icmp` + `br`）
2. `brcond chain, setcc(...), dest`

两种都处理很麻烦。RISC-V 后端的做法是：

```cpp
setOperationAction(ISD::BR_CC, MVT::i32, Expand);   // 统一变成 setcc + brcond
setOperationAction(ISD::BRCOND, MVT::Other, Custom); // 再由目标接管
```

`lowerBRCOND` 把 `setcc` 的语义"抬"到分支上：

```cpp
SDValue Ops[] = {chain, LHS, RHS, condcode, dest};
return DAG.getNode(RV32IISD::BR_CC, ...);
```

这样我们只需要处理 `brcond` 一种形态。

## 关键点 2：条件码规范化

RV32I 只有 `BLT/BGE/BLTU/BGEU`，没有 `BGT/BLE`。
所以 `a > b` 要改写成 `b < a`：

```cpp
case ISD::SETGT:  CC = ISD::SETLT;  std::swap(LHS, RHS); break;
case ISD::SETLE:  CC = ISD::SETGE;  std::swap(LHS, RHS); break;
case ISD::SETUGT: CC = ISD::SETULT; std::swap(LHS, RHS); break;
case ISD::SETULE: CC = ISD::SETUGE; std::swap(LHS, RHS); break;
```

## 关键点 3：`bb` 操作数

分支指令的最后一个操作数在 `.td` 里声明为：

```tablegen
def simm13_lsb0 : Operand<OtherVT>;
```

但在模式里用的是 `bb:$dst`：

```tablegen
def : Pat<(rv32i_brcc GPR:$rs1, GPR:$rs2, Cond, bb:$dst),
          (Inst GPR:$rs1, GPR:$rs2, bb:$dst)>;
```

`bb` 是 `ISD::BasicBlock` 的模式叶子。经过指令选择后它变成
`MachineOperand::MO_MachineBasicBlock`，最终由 `AsmPrinter::lowerOperand`
转成 `MCSymbolRefExpr`，打印为 `.LBB...`。

## 关键点 4：为什么需要 `PseudoBR`

直觉写法：

```tablegen
def : Pat<(br bb:$dst), (JAL X0, bb:$dst)>;   // 报错
```

报错：

```
Instruction 'JAL' was provided 2 operands but expected only 1!
```

因为 `JAL` 的 `$rd` 是 **out operand**，而模式输出只提供 **in operands**。
解决办法是定义一个没有目的寄存器的指令：

```tablegen
def PseudoBR : RVInst<(outs), (ins simm21_lsb0:$imm20), "j", "$imm20"> {
  let Inst = 0x0000006F; // jal x0, 0
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
};
def : Pat<(br bb:$dst), (PseudoBR bb:$dst)>;
```

> 这就是 RISCV 后端里 `PseudoBR` 存在的原因。

## 复现

```bash
ninja -C build-rv32i llc
./build-rv32i/bin/llc -mtriple=rv32i-unknown-elf -O2 /tmp/cf.ll -o -
```

## 观察点

```bash
# 看 BR_CC 节点与 BccPat 的匹配
./build-rv32i/bin/llc -mtriple=rv32i /tmp/cf.ll -debug-only=isel -o /dev/null

# 看 phi 消除与分支
./build-rv32i/bin/llc -mtriple=rv32i /tmp/cf.ll -print-after=phi-elimination -o /dev/null
```

## 本补丁的边界

- 分支目标距离尚未做 range 检查（`BranchRelaxation` 在对象文件阶段才有意义）；
- 没有 `select`（cmov）优化，会退化成分支；
- 仍然没有访存与栈帧（patch 05）。
