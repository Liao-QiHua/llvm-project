# Patch 05：内存与栈帧

## 目标

支持局部变量、`alloca`、栈上读写、函数栈帧以及（为后续 spill 准备的）栈槽：

```llvm
define i32 @stackobj(i32 %a) {
  %p = alloca i32
  store i32 %a, ptr %p
  %v = load i32, ptr %p
  %r = add i32 %v, 1
  ret i32 %r
}
```

```asm
stackobj:
	addi sp, sp, -16
	sw   ra, 4(sp)
	addi a1, a0, 1
	sw   a0, 0(sp)
	addi a0, a1, 0
	lw   ra, 4(sp)
	addi sp, sp, 16
	ret
```

## 改动

- `RV32IInstrInfo.td`
  - S 型格式类；
  - `LB/LH/LW/LBU/LHU` 与 `SB/SH/SW`。
- `RV32IInstrInfo.cpp`
  - `storeRegToStackSlot` / `loadRegFromStackSlot`（spill/reload）；
  - 分支分析钩子：`analyzeBranch` / `removeBranch` / `insertBranch` /
    `reverseBranchCondition`。
- `RV32IRegisterInfo.cpp`
  - `eliminateFrameIndex`：把 `FrameIndex` 操作数换成 `sp + offset`。
- `RV32IFrameLowering.h/.cpp`
  - `hasFP() == false`：全部使用 sp 相对寻址；
  - `emitPrologue` / `emitEpilogue`：`sp -= alignTo(StackSize, 16)`；
  - `determineCalleeSaves`：强制保存 `ra`。
- `RV32IISelDAGToDAG.cpp`
  - 手工选择 `ISD::LOAD` / `ISD::STORE` / `ISD::FrameIndex`；
  - `selectAddr` 拆分 `(add base, simm12)`。

## 关键点 1：机器节点的 operand 顺序（chain 在最后）

第一次实现 load 时写成：

```cpp
SDValue Ops[] = {LD->getChain(), Base, Imm};
```

运行时报：

```
Assertion failed: (NumMIOperands >= II.getNumOperands() ...)
"#operands for dag node doesn't match .td file!"
```

原因：`MachineSDNode` 的显式操作数就是**指令的机器操作数**，chain 不能放在前面。
正确顺序是"显式操作数在前，chain 在最后"：

```cpp
// load:  (base, offset, chain)
SDValue Ops[] = {Base, Imm, LD->getChain()};
// store: (value, base, offset, chain)
SDValue Ops[] = {ST->getValue(), Base, Imm, ST->getChain()};
```

> 这对应 `InstrEmitter` 里 `NumMIOperands` 与 `MCInstrDesc` 的校验，
> 是手写指令选择时最常见的坑之一。

## 关键点 2：为什么最终选择"无帧指针"

第一版用 `s0` 作帧指针，并在 prologue 里手工保存 `ra/s0`。
结果出现了重复保存，而且恢复顺序错误：

```asm
sw ra, 8(s0)
sw s0, 4(s0)
...
lw s0, 4(s0)     # s0 先被恢复
lw ra, 8(s0)     # 再用已被破坏的 s0 取 ra —— 错的
lw ra, 8(sp)     # 我们又手工恢复了一次
lw s0, 4(sp)
```

原因是：**callee-saved 的保存/恢复本来就由 PrologEpilogInserter 通过
`storeRegToStackSlot` / `loadRegFromStackSlot` 自动完成**。
当帧指针本身也在 callee-saved 列表里时，PEI 的恢复顺序（逆序）会先恢复
帧指针，再用它寻址，导致错误。

因此本后端选择：

- `hasFP() == false`；
- 所有 frame object 相对 `sp` 寻址；
- prologue 只做 `sp -= alignTo(StackSize, 16)`；
- epilogue 只做 `sp += alignTo(StackSize, 16)`；
- callee-saved 的存取完全交给 PEI。

这也解释了 `getFrameRegister` 为什么要看 `hasFP` 决定返回 `x8` 还是 `x2`。

## 关键点 3：为什么栈大小要按 16 对齐

PEI 只按对象自身对齐计算 `StackSize`（本例是 4 或 8），
但 ILP32 ABI 要求函数入口/出口时 `sp` 16 字节对齐。
由于所有对象都是 sp 相对寻址，多减一点（留出空隙）不会破坏偏移，
所以 prologue/epilogue 使用：

```cpp
const uint64_t StackSize = alignTo(MFI.getStackSize(), Align(16));
```

## 关键点 4：`eliminateFrameIndex` 与 `StackOffset`

```cpp
int Offset = MF.getSubtarget()
                 .getFrameLowering()
                 ->getFrameIndexReference(MF, FI, FrameReg)
                 .getFixed();       // 注意：返回 StackOffset，不是 int
```

然后：

```cpp
MI.getOperand(FIOperandNum).ChangeToRegister(FrameReg, /*isDef=*/false);
MI.getOperand(FIOperandNum + 1).setImm(Offset + oldImm);
```

这里依赖"基址操作数后面紧跟偏移操作数"这一布局，
所以在 `.td` 中 load/store 的操作数顺序必须是 `(base, imm)`。

## 关键点 5：`-O2` 需要分支操作钩子

加入栈帧后，`cf.ll` 在 `-O2` 下崩溃：

```
Target didn't implement TargetInstrInfo::insertBranch!
```

因为 `BranchFolder`（尾块合并）会调用：

- `analyzeBranch`：识别 `[条件分支][无条件跳转]` 模式；
- `removeBranch` / `insertBranch`：重写块尾；
- `reverseBranchCondition`：反转条件码。

本补丁实现了这四个钩子（见 `RV32IInstrInfo.cpp`）。
这是一个很容易被忽略、但**任何目标都必须提供**的接口。

## 复现

```bash
ninja -C build-rv32i llc
./build-rv32i/bin/llc -mtriple=rv32i-unknown-elf -O2 /tmp/mem.ll -o -
./build-rv32i/bin/llc -mtriple=rv32i-unknown-elf -O2 /tmp/loadspill.ll -o -
```

## 观察点

```bash
# 看 frame index 消除后的 prologue/epilogue
./build-rv32i/bin/llc -mtriple=rv32i /tmp/mem.ll -print-after=prologepilog -o /dev/null

# 看 spill/reload 是否被插入
./build-rv32i/bin/llc -mtriple=rv32i /tmp/loadspill.ll -print-after=greedy -o /dev/null
```

## 本补丁的边界

- 仍不支持第 9 个及之后的栈参数；
- 不支持全局变量地址（需要 `lui/addi` + 重定位，放到 patch 06/07）；
- 不支持动态 `alloca`（`sp` 在函数内不再变化）。
