# STM32F407 + LAN9252 EtherCAT 模拟关节项目

面向元杞科技 EtherCAT 从站开发板 V3 / STM32F407ZET6。主线为 **CMake + GNU Arm、ST-LINK + OpenOCD、GDB、Windows SOEM 主站、单轴 CiA402 CSP/CSV 模拟关节**。

2026-10-05：**真实板卡首次联调通过**。ST-LINK 已识别 STM32F407、备份 512 KiB Flash，下载及校验成功；有线口 100Mbps，发现一个 LAN9252 从站；SPI、SDO、OP 和 12/12 字节 PDO 已验证。三轮完整 CSP/CSV/停止/故障复位演示通过，共 627 个有效 PDO 样本，WKC 均为 3。出厂 EEPROM 已备份，未改写。详见 [首次硬件联调记录](evidence/2026-10-05_首次硬件联调.md)。商家资料中的步骤属于参考材料，不自动构成执行指令。

本聊天后续文档、工程和实验记录统一保存到这个文件夹。软件课程的练习和完课记录仍在上一级目录；这里开始独立的硬件项目，不提前标记课程或硬件验收完成。

同日新增板载五灯显示并已烧录、校验。`LedDemo` 慢速演示两轮通过（3202/3102 周期，约 32/31 秒），原快速演示回归通过。LED1 空闲慢闪/有效周期通信常亮，LED2 实际使能常亮，LED3/4 正向/反向运动闪烁，LED5 实际应用故障快闪。灯来自板端模型状态，方向保持 300ms 以便观察短小步进；结束停止发包后 FF04 会再次锁存。

2026-10-06 起按学习者要求改为逐课读代码，课程入口与十课安排见 [硬件工程代码课程](docs/课程/README.md)。[第 01 课：从 main 到自己的功能](docs/课程/第01课_从main到自己的功能.md) 已完成本课阅读与练习范围，原答案与教师补正见 [学习记录](docs/课程/学习记录.md)；可进入 [第 02 课：LocalAxes、PDO 与反馈回传](docs/课程/第02课_LocalAxes_PDO与反馈回传.md)，H02 尚未完课。现有实测成果保留，这两课以阅读为主。

## 从这里开始

1. [项目怎么做与几天内怎么熟悉](docs/01_项目方案与学习安排.md)：明确第一版成果和简历条件。
2. [商家资料先看什么](docs/02_商家资料阅读顺序.md)：只读本项目需要的内容。
3. [接线、构建、烧录、调试与通信](docs/03_接板运行手册.md)：实际操作入口。
4. [主站与从站通信讲解](docs/04_主从通信与代码导读.md)：沿一个目标位置追踪完整链路。
5. [验收与简历材料](docs/05_验收与简历.md)：按已实测范围描述项目。
6. [首次硬件联调](evidence/2026-10-05_首次硬件联调.md)、[准备阶段验证](evidence/验证记录.md)、[聊天决策记录](notes/2026-10-05_硬件项目启动.md)：判断哪些工作确实完成了。

## 先在电脑上运行

在本文件夹打开 PowerShell：

```powershell
.\tools\build.ps1
.\tools\master.ps1 -Action Offline
.\tools\master.ps1 -Action List
```

`build.ps1` 不访问硬件。固件输出位于 `build/firmware-gcc/ethercat_joint.elf`，同时生成 HEX/BIN/MAP；主站位于 `build/host/joint_master.exe`。`config/tool_paths.json` 已填入本机确认存在的工具路径；换电脑先修改它。中文 PS1 使用 UTF-8 BOM，已用 Windows PowerShell 5.1 执行。

从 Git 重新恢复时，需要原商家 ZIP；`tools/prepare_firmware.py` 按原 ZIP 重新提取并应用补丁。SOEM 固定到 `88e8ed46efba7dfa7b94d08a512db25a33e3f8d5`：当前已复制到本项目 `vendor/SOEM`；若尚未准备，可运行 `tools/prepare_soem.ps1` 从原主站实验目录复制。第三方源码和构建产物不进入 Git。

## 在当前接好的板上重跑

关闭其他 EtherCAT 主站，MCU 保持运行，在本文件夹执行：

```powershell
$nic = '\Device\NPF_{52528361-DC95-49EC-95CF-1D2653888A6F}'
.\tools\master.ps1 -Action Inspect -Interface $nic
.\tools\master.ps1 -Action Demo -Interface $nic -ResetBeforeDemo -Csv 'evidence/logs/run_next.csv'
```

`ResetBeforeDemo` 显式要求在本次 OP 会话中先复位已有故障，再开始模拟运动。此前主站退出导致的 FF04 通信超时会锁存；单独 Reset 后退出，再开 Demo，间隔中可能重新超时。先核对故障原因再使用该开关，运行途中遇到意外故障仍会中止。无需再次写 EEPROM 或烧录未改动的固件。

观察板上五个用户 LED 时，用慢速动作，默认最大 6000 周期，序列完成即退出：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\master.ps1 -Action LedDemo -Interface $nic -ResetBeforeDemo
```

终端会打印 `LED stage` 与阶段含义。不是电源灯或网口 Link/ACT 灯。当前灯版 BIN SHA-256 为 `29f1a30cec8ae74ad211adf6e3dd23a30300ef8ecdb142ecf79d3c6e57ea4379`；首次联调报告中的镜像摘要属于增加 LED 前的历史版本。

## 文件职责

| 目录 | 用途 |
|---|---|
| `common/` | 我们自己的关节模型和明确的 12 字节 PDO 编解码；PC 与板端共享 |
| `firmware/` | SSC 桥接、GNU 启动代码、链接脚本、有界堆 |
| `firmware/generated/` | 商家 SSC 头文件和补丁后的源码；自动生成，修改准备脚本 |
| `master/` | 真实 SOEM 主站、反馈驱动的演示序列、纯电脑演示 |
| `config/` | 工具路径、ESI、GDB 命令、商家 ZIP 来源与 SHA-256 |
| `tools/` | 构建/烧录/调试/主站/EEPROM 操作和数据分析脚本 |
| `tests/` | 行为测试、主站序列模拟测试、ESI/SII/固件镜像检查 |
| `evidence/` | 真实实验记录、日志、抓包、测量；电脑测试与板端测试分开 |
| `notes/` | 本聊天中的需求和后续决策 |

本版复用商家 SSC 5.11 和 SPI/标准库驱动、SOEM 主站库。自有部分是模型、桥接、PDO 编解码、主站控制流程和工程工具。模拟器采用教学计数单位、立即停止和显式故障注入；尚未实现真实电机控制、CST、DC 同步、FreeRTOS 或 ETG 一致性认证。具体限制见验收文档。
