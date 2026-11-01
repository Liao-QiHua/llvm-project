# 从零移植一个 LLVM 后端：RV32I 增量教程

本目录记录在 LLVM 中**从零新增一个后端**的完整过程。

目标后端叫 `RV32I`：

- 指令集语义与 RISC-V **RV32I** 基本整数指令集等价（同样的 32 位编码、同样的 ABI 寄存器约定）；
- 但它是 LLVM 中的一个**独立目标**：独立 triple `rv32i-...`、独立命名空间、独立 TableGen 描述、独立 MC 层，
  不依赖 `llvm/lib/Target/RISCV`；
- 因此可以把"移植一个新 ISA 后端"的每一步都看清楚，而不会被 RISC-V 后端里大量的扩展代码淹没。

> 一句话：**一个和 RV32I 功能一样、但在 LLVM 里完全"另起炉灶"的后端。**

---

## 1. 分支与构建

```bash
git switch rv32i-backend-port
```

专用最小构建目录（只编译 RV32I 一个目标，避免全量构建）：

```bash
cmake -S llvm -B build-rv32i -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_TARGETS_TO_BUILD=RV32I \
  -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF \
  -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_INCLUDE_DOCS=OFF \
  -DLLVM_BUILD_UTILS=OFF -DLLVM_ENABLE_BINDINGS=OFF \
  -DLLVM_ENABLE_TERMINFO=OFF -DLLVM_ENABLE_ZLIB=OFF \
  -DLLVM_ENABLE_ZSTD=OFF -DLLVM_ENABLE_LIBXML2=OFF \
  -DLLVM_ENABLE_CURL=OFF -DLLVM_ENABLE_LIBEDIT=OFF \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

ninja -C build-rv32i llc
```

验证目标已注册：

```bash
./build-rv32i/bin/llc --version | grep -A1 rv32i
```

---

## 2. 补丁序列（每个补丁都能独立编译）

| # | 补丁 | 新增能力 | 验证方式 |
|---|---|---|---|
| 01 | `[RV32I] Add the target skeleton and lower \`ret void\`` | Triple/CMake 注册、寄存器文件、最小 MC 层、空函数返回 | `llc retvoid.ll` → `ret` |
| 02 | 常量与返回值 | `ADDI`/`LUI`、`li` 物化、返回值搬运 | `ret i32 42` → `li a0, 42; ret` |
| 03 | 参数与整数 ALU | 寄存器分配、`add/sub/and/or/xor/slt/sltu/shift` 及其模式 | `add3(a,b,c)` |
| 04 | 控制流 | 条件分支、比较、无条件跳转、循环 | `if/else`、`while` |
| 05 | 内存与栈帧 | load/store、alloca、frame index、prologue/epilogue、spill | 局部数组、多变量 |
| 06 | 函数调用 | `LowerCall`、`j`/`call`、callee-saved、递归 | `fact(n)` |
| 07 | 目标文件 | MCCodeEmitter、AsmBackend、ELF、重定位 | `-filetype=obj` + `llvm-readobj` |
| 08 | 全局地址 / 汇编器 / 反汇编器 | `lui/addi` + HI20/LO12、asm matcher、disassembler | `llvm-mc -show-encoding` |
| 09 | 完整化 | 全部 RV32I 指令、调度模型、对照测试 | 与 `riscv32` 输出对照 |

每个补丁的详细说明见同目录下的 `01-*.md`、`02-*.md` … 文件，包含：

- 改了哪些文件、为什么；
- 关键代码片段；
- 如何复现与预期输出；
- 该步骤在 LLVM 后端流程中的位置。

---

## 3. 这个后端不是什么

- 不是 RISC-V 后端的复制品：没有 M/A/F/D/V 扩展，没有压缩指令，没有 RISC-V 特有的 pass；
- 不追求与 GCC/binutils 完全一致的汇编语法细节（但编码遵循 RV32I，因此生成的目标文件在字节层面与 RV32I 一致）；
- 目标是**可读、可增量复现**，每一层都只引入当时必需的最小实现。

---

## 4. 建议的阅读方式

1. 按补丁顺序读文档，同时在另一个终端 `git show <commit>` 看 diff；
2. 每个补丁先跑"验证方式"里的命令，确认行为；
3. 遇到不理解的点，用 `llc -print-after=<pass>` / `-debug-only=isel` 观察中间结果。

---

## 5. 进度

- [x] 01 骨架与 `ret void`
- [x] 02 常量与返回值
- [x] 03 参数与整数 ALU
- [x] 04 控制流
- [x] 05 内存与栈帧
- [x] 06 函数调用
- [x] 07 目标文件
- [ ] 08 全局地址 / 汇编器 / 反汇编器
- [ ] 09 完整化与对照测试

## 6. 当前状态与已知限制

已经打通：`IR → SelectionDAG → 指令选择 → 调度 → 寄存器分配 → 栈帧 → 汇编 / ELF 目标文件`。

尚未实现（按建议顺序）：

1. **全局变量寻址**：`lui %hi(sym)` / `addi %lo(sym)`，`R_RISCV_HI20/LO12_I/LO12_S`；
   `LowerGlobalAddress` + 可重定位操作数 + `%hi/%lo` 打印。
2. **栈参数**：第 9 个及之后的参数（`LowerFormalArguments`/`LowerCall` 中的栈槽分配）。
3. **间接调用 / 尾调用 / varargs**。
4. **汇编器与反汇编器**：`-gen-asm-matcher` + `RV32IAsmParser`，
   `-gen-disassembler` + `RV32IDisassembler`（届时 `llvm-mc` 才能工作）。
5. **指令集完整性**：目前覆盖 RV32I 的算术/逻辑/移位/分支/访存/跳转/调用；
   文中的 `JAL` 已定义但代码生成路径主要用 `PseudoBR`/`PseudoCALL`。
6. **调度模型**：目前是单发射占位模型（`RV32ISchedModel`），
   可参照 `RISCVSchedSiFive7.td` 增加真实的 `WriteRes`/`ProcResource`。
7. **ABI 细节**：`gp/tp` 初始化、`R_RISCV_RELAX`、`.riscv.attributes` 等。

继续实现时，建议严格沿用本目录的模式：**一个 commit 只做一层，提交前跑一遍
`ninja -C build-rv32i llc` 与 `docs/rv32i-port` 中的验证命令**。

---

## 7. 如果纯手工实现，需要多久（单人估时）

前提：单人、从零写、每个补丁都要"能编译 + 用 `llc` 验证输出"。
单位为人日（8 小时/天）。"熟手"指已经写过或读过 LLVM 后端代码；
"新手"指第一次接触 TableGen / SelectionDAG / MC 层。

| 补丁 | 改动量 | 核心难点 | 熟手 | 新手 |
|---|---|---|---|---|
| 01 骨架 + `ret void` | ~1670 行 / 36 文件 | 90% 是 CMake/TableGen 样板，但要跑通构建：`Target` 定义、`AsmPrinter` 抽象类、pass 注册顺序、Triple 联调 | 2–3 | 5–8 |
| 02+03 常量 / 参数 / 整数 ALU | ~610 行 / 8 文件 | 操作数类型系统（`simm12`/`uimmlog2xlen`）、Pattern 写法、`LowerFormalArguments`/`LowerReturn` | 2–4 | 4–7 |
| 04 控制流 | ~350 行 / 5 文件 | `BR_CC` 与 `BRCOND` 的分工、条件规范化（`setle` → `setgt` 交换）、`analyzeBranch`/`insertBranch` 四个钩子 | 2–3 | 4–6 |
| 05 内存 + 栈帧 + spill | ~630 行 / 9 文件 | `FrameLowering`、`eliminateFrameIndex`、spill 与 PEI 的交互（容易出现"能跑但结果错"的 bug） | 3–5 | 6–10 |
| 06 直接调用 | ~230 行 / 6 文件 | 调用约定、`CopyToReg`/`CALL`/`CopyFromReg` 链路、callee-saved 与 `ra` 保存 | 2–3 | 4–6 |
| 07 ELF + 重定位 | ~640 行 / 11 文件 | `AsmBackend`/`ELFObjectWriter`/`FixupKinds`，以及"EncoderMethod 只对绑定到 `Inst` 位域的操作数生效"这类隐性规则 | 3–4 | 6–9 |
| 文档 | 22 行 | — | 1 | 2 |
| **合计** | ~4100 行 | | **15–22 人日<br>≈ 3–4.5 周** | **31–48 人日<br>≈ 1.5–2.5 个月** |

影响估时的几个主要因素：

- **是否有参考实现**：允许照抄一个成熟后端（Mips / Cpu0 / AVR）改造，
  01、02/03、07 各能省 30–50%；纯按 LangRef 与头文件摸索则要往上限靠。
- **构建成本**：首次配置 + 全量编译约 1300 个目标（`-j10` 上约半小时到一小时），
  之后增量编译 30 秒–2 分钟。人工调试单次"改一行 → 编译 → 跑 `llc`"通常 1–3 分钟，
  一天只能验证一二百次假设，这才是真正的瓶颈，而不是写代码本身。
- **时间黑洞不在写代码**：本项目中改动量最小的几轮往往耗时最长，例如
  用 `SDNPVariadic` 让返回值不被 DCE 删除、手写 load/store 节点时 chain 必须放在最后、
  `getFrameIndexReference` 返回 `StackOffset` 需要 `.getFixed()`、
  `rv32i_call` 的 `SDTypeProfile` 为空导致 TableGen 报错。人工同样会卡在这些点上，
  但缺少"翻头文件 + 立刻重编"的循环速度，实际耗时通常还要上浮 20–30%。
- **补丁切分方式**：现在是"边写边切、每步都能编译"的最省力顺序；
  若先写完再做回溯式切分，总分要多 30–50%，因为每个切点都要重新验证。

如果目标是**学习理解**而非交付：把 01–04 走完（能算算术、能分支）大约 1 周，
就足以建立完成后端的心智模型；05 与 07 更适合"先懂原理、按需动手"。

