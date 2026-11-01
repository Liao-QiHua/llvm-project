# Patch 06：函数调用

## 目标

支持直接函数调用与递归：

```llvm
define i32 @callee(i32 %a, i32 %b) { %r = add i32 %a, %b  ret i32 %r }

define i32 @caller(i32 %x) {
  %r = call i32 @callee(i32 %x, i32 7)
  %s = add i32 %r, 1
  ret i32 %s
}
```

```asm
caller:
	addi sp, sp, -16
	sw   ra, 0(sp)
	call callee
	addi a0, a0, 1
	lw   ra, 0(sp)
	addi sp, sp, 16
	ret
```

## 改动

- `RV32IInstrInfo.td`
  - `SDT_RV32ICall` / `rv32i_call` 节点；
  - `PseudoCALL`（`isCall = 1`、`Defs = [X1]`、打印为 `call symbol`）。
- `RV32IISelLowering.cpp`
  - 实现 `LowerCall`：参数 `CopyToReg` 到 `a0-a7`，发 `RV32IISD::CALL`，
    返回值从 `a0` `CopyFromReg`。
- `RV32IISelDAGToDAG.cpp`
  - 在 `Select()` 里直接把 `RV32IISD::CALL` 选成 `PseudoCALL`。

## 关键点 1：调用前后的数据流

```cpp
for (unsigned I = 0; I < Outs.size(); ++I)
  Chain = DAG.getCopyToReg(Chain, DL, ArgRegs[I], OutVals[I]);

Chain = DAG.getNode(RV32IISD::CALL, DL, MVT::Other, Callee, Chain);

if (!Ins.empty()) {
  SDValue V = DAG.getCopyFromReg(Chain, DL, RV32I::X10, Ins[0].VT);
  InVals.push_back(V);
  Chain = V.getValue(1);
}
```

- `CopyToReg` 产生的 chain 保证参数在调用前写好；
- 调用后的 chain 再交给 `CopyFromReg`，保证返回值在调用后读取。

## 关键点 2：为什么 CALL 用手写选择而不是 `Pat`

尝试写：

```tablegen
def : Pat<(rv32i_call tglobaladdr:$func), (PseudoCALL tglobaladdr:$func)>;
```

运行时报：

```
Cannot select: t10: ch = RV32IISD::CALL TargetGlobalAddress:i32<ptr @callee> 0, t9
```

原因是符号操作数的类型（`i32`）与模式叶子 `tglobaladdr` 的推断类型（`iPTR`）
不一致，`SDTCisVT` 约束无法同时满足。

对于这种"操作数不是普通寄存器/立即数"的节点，手写选择更简单可靠：

```cpp
case RV32IISD::CALL: {
  SDValue Ops[] = {N->getOperand(0), N->getOperand(1)};
  SDNode *New = CurDAG->getMachineNode(RV32I::PseudoCALL, DL, MVT::Other, Ops);
  ReplaceNode(N, New);
  return;
}
```

## 关键点 3：调用约定如何影响寄存器分配

`PseudoCALL` 上的：

```tablegen
let isCall = 1;
let Defs = [X1];
```

做了两件事：

1. `Defs = [X1]` 表示调用会写 `ra`；
2. `isCall = 1` 让 `InstrEmitter` 给调用插入一个 **regmask**，
   内容来自 `RV32IRegisterInfo::getCallPreservedMask`
   （即 `CSR_ILP32_RegMask`）。

于是递归函数 `rec` 会生成：

```asm
addi a1, a0, -1
addi a2, a0, 0      # 把 n 保存到 callee-saved 的 s2 类寄存器
addi a0, a1, 0
call rec
add  a0, a0, a2     # 调用后仍然可用
```

如果 regmask 不对，`a2`（caller-saved）在调用后就会被当作有效，
生成错误代码——这是"程序能编译但运行结果错"的典型来源。

## 关键点 4：`call` 只是汇编层面的伪指令

当前 `PseudoCALL` 的 `AsmString` 是 `"call $func"`，
所以汇编输出是：

```asm
call callee
```

它还没有被展开成 `jal ra, callee`，也没有重定位信息。
这部分在 Patch 07（目标文件/重定位）完成。

## 复现

```bash
ninja -C build-rv32i llc
./build-rv32i/bin/llc -mtriple=rv32i-unknown-elf -O2 /tmp/call.ll -o -
```

## 本补丁的边界

- 只支持直接调用（间接调用需要 `jalr` 与寄存器操作数）；
- 不支持尾调用、varargs、第 9 个及之后的栈参数；
- 不做 `callseq_start/end`，因此还没有"调用会改变 sp"的建模
  （当前所有参数都在寄存器里，暂时不影响正确性）。
