# Patch 02：常量与返回值

## 目标

让函数能够返回一个 32 位整数常量：

```llvm
define i32 @fortytwo() { ret i32 42 }
define i32 @minus_one() { ret i32 -1 }
```

```asm
fortytwo:
	addi a0, zero, 42
	ret
minus_one:
	addi a0, zero, -1
	ret
```

## 改动

- `RV32IInstrInfo.td`
  - 新增 R/I/U 三种格式类；
  - 新增 `LUI`、`ADDI`、`ADD` 三条指令（含 32 位编码与汇编语法）；
  - 新增 `simm12` / `uimm20` 操作数（同时是 `ImmLeaf`）；
  - 新增 `add` 的两条 DAG 模式；
  - `rv32i_ret` 改为 `SDNPVariadic`。
- `RV32IInstrInfo.cpp`
  - 实现 `copyPhysReg`：`mv rd, rs` ⇒ `addi rd, rs, 0`。
- `RV32IISelDAGToDAG.cpp`
  - `Select()` 处理 `ISD::Constant`，物化为 `addi rd, x0, imm`。
- `RV32IISelLowering.cpp`
  - `LowerReturn` 把返回值 `CopyToReg` 到 `a0`，并把 `a0` 作为返回节点的操作数。
- `RV32IInstPrinter.cpp`
  - 打印 ABI 寄存器名（`a0`、`sp`…），而不是 `x10`、`x2`。

## 关键点 1：为什么 `CopyToReg` 会"凭空消失"

第一版实现里，`LowerReturn` 只做了：

```cpp
if (!Outs.empty())
  Chain = DAG.getCopyToReg(Chain, DL, RV32I::X10, OutVals[0]);
return DAG.getNode(RV32IISD::RET, DL, MVT::Other, Chain);
```

结果生成：

```asm
fortytwo:
	ret          # 42 不见了
```

原因是返回节点只消费了 chain，没有"使用" `a0`。
后面的机器级 DCE 看到 `a0` 没有被任何指令使用，就把 `COPY a0, ...` 删掉了。

正确做法（AVR/RISC-V 等后端同款）是让返回节点显式携带返回寄存器：

```tablegen
def rv32i_ret : SDNode<"RV32IISD::RET", SDTNone,
                       [SDNPHasChain, SDNPVariadic]>;
```

```cpp
SmallVector<SDValue, 4> RetOps(1, Chain);
if (!Outs.empty()) {
  Chain = DAG.getCopyToReg(Chain, DL, RV32I::X10, OutVals[0]);
  RetOps[0] = Chain;
  RetOps.push_back(DAG.getRegister(RV32I::X10, MVT::i32));
}
return DAG.getNode(RV32IISD::RET, DL, MVT::Other, RetOps);
```

这也是一个通用规律：**物理寄存器的"隐式使用"必须在 DAG 层面表达出来**，
否则优化器无法知道它是 live-out。

## 关键点 2：常量物化放在 `Select()` 里

`ISD::Constant` 不通过 TableGen 模式匹配，而是在 `Select()` 里手写：

```cpp
bool RV32IDAGToDAGISel::selectConstant(SDNode *N) {
  auto *CN = cast<ConstantSDNode>(N);
  int64_t Imm = CN->getSExtValue();
  EVT VT = N->getValueType(0);
  SDLoc DL(N);

  if (!isInt<12>(Imm))
    report_fatal_error("...wider than 12 bits...");

  SDValue Ops[] = {CurDAG->getRegister(RV32I::X0, VT),
                   CurDAG->getTargetConstant(Imm, DL, VT)};
  ReplaceNode(N, CurDAG->getMachineNode(RV32I::ADDI, DL, VT, Ops));
  return true;
}
```

> 把常量物化写成手写代码而不是 `Pat`，是因为：
> 1. 大常量需要多指令序列（`lui` + `addi`），模式匹配难以表达；
> 2. 是否需要序列取决于上下文（能否符号扩展、后续使用者位数），
>    这正是 `RISCVISelDAGToDAG::selectImm` 存在的原因。

## 关键点 3：ABI 寄存器名

TableGen 生成的 `getRegisterName` 返回 `.td` 里的 `AsmName`（`x10`）。
我们在 `printOperand` / `printRegName` 里把它映射成 ABI 名：

```cpp
if (const char *ABIName = getRegisterABIName(MO.getReg()))
  O << ABIName;
else
  O << getRegisterName(MO.getReg());
```

## 复现

```bash
ninja -C build-rv32i llc
./build-rv32i/bin/llc -mtriple=rv32i-unknown-elf /tmp/const.ll -o -
```

## 观察点

```bash
# 选择前后的 DAG
./build-rv32i/bin/llc -mtriple=rv32i /tmp/const.ll \
  -debug-only=isel -o /dev/null

# 返回值的 COPY 是否还在
./build-rv32i/bin/llc -mtriple=rv32i /tmp/const.ll \
  -print-after=finalize-isel -o /dev/null
```

## 本补丁的边界

- 只支持 12 位有符号常量；
- 还不支持参数（`LowerFormalArguments` 遇到参数会报错）；
- `LUI` 已经定义但还没有被使用（下一步用于大常量与地址）。
