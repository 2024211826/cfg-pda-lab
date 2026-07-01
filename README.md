# 理论计算机科学基础实验

本仓库用于整理“上下文无关文法变换”和“PDA 到 CFG 转换”实验材料。README 只说明如何把程序跑起来；算法说明、测试设计和实验报告请看 `docs/`。

## 环境要求

- Windows 10/11
- MinGW g++，建议支持 C++17
- PowerShell 或 CMD

检查编译器：

```powershell
g++ --version
```

## 当前目录

```text
docs/      实验要求、分工和后续文档
src/       本实验源码
tests/     测试输入、测试脚本和输出记录
build/     编译输出目录，可重新生成
ppt/       课程课件
学长/      往届参考代码和报告，仅供阅读参考
运行结果截图/  实验报告中使用的运行截图
```

## 编译本项目

在项目根目录运行：

```powershell
New-Item -ItemType Directory -Force .\build | Out-Null
g++ -std=c++17 -Wall -Wextra -pedantic .\src\main.cpp .\src\cfg.cpp .\src\pda.cpp -o .\build\pda_cfg.exe
```

运行：

```powershell
.\build\pda_cfg.exe
```

也可以直接带模式参数运行：

```powershell
.\build\pda_cfg.exe cfg
.\build\pda_cfg.exe pda
```

注意：`学长/lab2` 代码存在越界访问、迭代器删除和非终结符重命名等风险，只建议阅读参考，不建议运行或作为最终提交程序。

## 程序模式

程序启动后会要求选择模式：

```text
1. Simplify CFG
2. Convert PDA to CFG and simplify
```

模式 1 用于上下文无关文法化简，推荐输入格式如下：

```text
N: S A B C D
T: a b c d
S: S
P:
S -> a | b A | B | c c D
A -> a b B | #
B -> a A
C -> d d C
D -> d d d
END
```

空串用 `#` 表示。每个符号之间建议用空格分隔，便于支持多字符符号。

模式 2 用于 PDA 到 CFG 转换并继续化简，推荐输入格式如下：

```text
Q: q0 q1
T: a b
G: B z0
q0: q0
z0: z0
F:
D:
q0 b z0 -> q0 B z0
q0 b B -> q0 B B
q0 a B -> q1 #
q1 a B -> q1 #
q1 # B -> q1 #
q1 # z0 -> q1 #
END
```

其中：

- `#` 在 PDA 输入符号位置表示空输入。
- `#` 在 PDA 压栈串位置表示弹出栈顶后不再压入新符号。
- 本实验按空栈接受处理，`F:` 可以为空。

## 快速运行样例

CFG 样例：

```powershell
@'
N: S A B C D
T: a b c d
S: S
P:
S -> a | b A | B | c c D
A -> a b B | #
B -> a A
C -> d d C
D -> d d d
END
'@ | .\build\pda_cfg.exe cfg
```

PDA 样例：

```powershell
@'
Q: q0 q1
T: a b
G: B z0
q0: q0
z0: z0
F:
D:
q0 b z0 -> q0 B z0
q0 b B -> q0 B B
q0 a B -> q1 #
q1 a B -> q1 #
q1 # B -> q1 #
q1 # z0 -> q1 #
END
'@ | .\build\pda_cfg.exe pda
```

也可以直接使用 `tests/` 中的输入文件运行：

```powershell
Get-Content .\tests\cfg_sample.txt | .\build\pda_cfg.exe cfg
Get-Content .\tests\pda_sample.txt | .\build\pda_cfg.exe pda
```

## 批量测试

测试输入位于 `tests/`，批量测试脚本为：

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\run_all.ps1
```

测试输出会保存到 `tests/output/`。

## 输出说明

CFG 模式输出 `Simplified CFG`。

PDA 模式先输出 `Equivalent CFG generated from PDA`，再输出 `Simplified CFG`。

输出包含非终结符集合、终结符集合、开始符号和产生式集合，格式适合直接截图放入实验报告。
