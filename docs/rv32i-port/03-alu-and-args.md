# Patch 03：参数与整数 ALU

## 目标

让后端支持函数参数和 RV32I 的整数运算：

```llvm
define i32 @add3(i32 %a, i32 %b, i32 %c) {
  %x = add i32 %a, %b
  %y = add i32 %x, %c
  ret i32 %y
}
```

```asm
add3:
	add a0, a0, a1
	add a0, a0, a2
	ret
```

## 改动

- `RV32IInstrInfo.td`
  - 新增 `SUB/AND/OR/XOR/SLT/SLTU`（R 型）；
  - 新增 `ANDI/ORI/XORI/SLTI/SLTIU`（I 型）；
  - 新增 `SLLI/SRLI/SRAI`（I 型移位）与 `SLL/SRL/SRA`（R 型移位）；
  - 新增 `uimmlog2xlen` 操作数（0..31）；
  - 为上述操作新增 DAG 模式。
- `RV32IISelLowering.cpp`
  - 实现 `LowerFormalArguments`：前 8 个 i32 参数进入 `a0`–`a7`。

## 关键点 1：参数如何进入 DAG

```cpp
static const MCPhysReg ArgRegs[] = {
    RV32I::X10, RV32I::X11, RV32I::X12, RV32I::X13,
    RV32I::X14, RV32I::X15, RV32I::X16, RV32I::X17};

for (const ISD::InputArg &Arg : Ins) {
  MVT VT = Arg.VT;
  Register VReg = MF.addLiveIn(ArgRegs[ArgNo], &RV32I::GPRRegClass);
  InVals.push_back(DAG.getCopyFromReg(Chain, DL, VReg, VT));
}
```

- `addLiveIn` 表示"函数入口处这个物理寄存器已经活跃"；
- `getCopyFromReg` 把它读成一个**虚拟寄存器值**，后续所有机器级 pass 都只看虚拟寄存器；
- 真正把它变成物理寄存器，是 patch 05 之后的寄存器分配工作。

## 关键点 2：操作数叶子（Operand + ImmLeaf）

```tablegen
def simm12 : Operand<i32>, ImmLeaf<i32, [{ return isInt<12>(Imm); }]>;
def uimmlog2xlen : Operand<i32>, ImmLeaf<i32, [{
  return Imm >= 0 && Imm < 32;
}]>;
```

一个 `def` 同时继承 `Operand` 与 `ImmLeaf`，意味着它既能出现在指令操作数列表里，
又能出现在 `Pat` 的输入模式里做常量范围判断。这是 RISC-V 后端里非常常见的手法。

于是：

```tablegen
def : Pat<(i32 (add GPR:$rs1, simm12:$imm)), (ADDI GPR:$rs1, simm12:$imm)>;
def : Pat<(i32 (shl GPR:$rs1, uimmlog2xlen:$imm)),
          (SLLI GPR:$rs1, uimmlog2xlen:$imm)>;
```

## 关键点 3：为什么没有 `sub x, imm` 的模式

直觉上应该写：

```tablegen
def : Pat<(i32 (sub GPR:$rs1, simm12:$imm)), (ADDI GPR:$rs1, ???)>;
```

但 TableGen 不允许在**输入模式**里用 `SDNodeXForm`（如取负），直接写会报：

```
Cannot use 'NegImm' in an input pattern!
```

实际上也不需要：`DAGCombiner` 会把

```
sub x, C   ==>   add x, -C
```

规范化掉，所以只需要寄存器-寄存器版本：

```tablegen
def : Pat<(i32 (sub GPR:$rs1, GPR:$rs2)), (SUB GPR:$rs1, GPR:$rs2)>;
```

> 这是一个很好的例子：**指令选择不是孤立的一步**，它依赖 combine/legalize 之后
> DAG 的规范形式。写模式前先确认 DAG 长什么样。

## 关键点 4：`zext i1` 自动消失

```llvm
%c = icmp slt i32 %a, %b
%z = zext i1 %c to i32
ret i32 %z
```

生成：

```asm
slt a0, a0, a1
ret
```

因为 `setcc` 被直接选成 `SLT`，而 `SLT` 的结果本身就是 0/1，
`setBooleanContents(ZeroOrOneBooleanContent)` 让 `zext` 被合法化为恒等。

## 复现

```bash
ninja -C build-rv32i llc
./build-rv32i/bin/llc -mtriple=rv32i-unknown-elf -O2 /tmp/alu.ll -o -
```

## 观察点

```bash
# 看参数如何变成 CopyFromReg
./build-rv32i/bin/llc -mtriple=rv32i /tmp/alu.ll -print-after=finalize-isel -o /dev/null

# 看寄存器分配结果
./build-rv32i/bin/llc -mtriple=rv32i /tmp/alu.ll -print-after=greedy -o /dev/null
```

## 本补丁的边界

- 最多 8 个整数参数；第 9 个参数会触发 `stack arguments are implemented in a later patch`；
- 没有分支/循环（patch 04）；
- 没有访存与栈帧（patch 05）。
