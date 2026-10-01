# Windows SOEM 主站学习环境

本项目确定先使用 Windows + SOEM + Npcap + 笔记本自带 Realtek 有线网卡。第一阶段目标是学会扫描从站、读取对象字典和交换 PDO；严格 1 ms 实时周期留到后续测量与实时平台课程。

准备日期：2026-10-01。主站环境已经完成编译和网卡枚举，尚未连接 EtherCAT 从站，因此没有验证从站发现、网卡收发、SDO、PDO、OP 或周期抖动。

## 1. 各部分分别做什么

```text
主站 C 程序：填写目标值、读取反馈、请求状态转换
    ↓ 调用库函数
SOEM：组织 EtherCAT 帧、配置从站、处理 PDO / SDO
    ↓ Windows 的抓包 / 发包接口
Npcap：让程序访问以太网帧
    ↓
Realtek 有线网卡 → 网线 → 从站模块的 EtherCAT IN
                              ↓ LAN9252 ESC
                              ↓ SPI / PDI
                           STM32H743
                              ↓
                       从站协议栈与关节应用
```

SOEM 是 C 库和示例源码，不是 TwinCAT 那样的图形化软件。Npcap 是网卡访问组件，不是 EtherCAT 协议栈。Wireshark 是抓包工具，不负责当主站。

EtherCAT 使用二层以太网帧，EtherType 是 0x88A4。这条直接连接的 EtherCAT 链路不需要给从站设置 IP，也不依靠 TCP/UDP。上网可以继续使用 Wi-Fi；实验时有线接口直接接从站。

## 2. 当前电脑的软件情况

| 软件 / 硬件 | 当前结果 | 是否需要新安装 |
|---|---|---|
| GCC | Qt 自带 MinGW，13.1.0，已用于编译 SOEM | 不需要 |
| CMake | 4.0.1，已完成配置 | 不需要 |
| mingw32-make | 已有，已完成构建 | 不需要 |
| Git、VS Code | 已有 | 不需要 |
| Npcap | 服务 Running，DLL 可用，网卡枚举成功 | 暂不重装 |
| Realtek Gaming 2.5GbE Family Controller | 已出现在 SOEM 枚举列表 | 暂不换网卡 |
| Wireshark | 未核实安装情况 | 接板后抓包需要时安装 |
| TwinCAT | 本阶段不用 | 不需要 |

Npcap 的驱动已安装；SOEM 此版本附带 Windows 编译所需的 pcap 头文件与导入库，本次构建没有额外下载 SDK。以后自己开发其他 pcap 程序，再按需要使用官方 Npcap SDK。

## 3. 固定本次 SOEM 版本

源码位置：`vendor/SOEM`。

本次下载的提交：`88e8ed46efba7dfa7b94d08a512db25a33e3f8d5`，CMake 中的项目版本为 2.0.0。源码未修改。网上旧版教程的 API、文件位置和命令可能不同，先以本地版本为准。

现有源码已下载，不用重复 clone。在另一台电脑重新准备时，可执行：

```powershell
git clone https://github.com/OpenEtherCATsociety/SOEM.git .\EtherCAT_Master_Lab\vendor\SOEM
git -C .\EtherCAT_Master_Lab\vendor\SOEM checkout 88e8ed46efba7dfa7b94d08a512db25a33e3f8d5
```

SOEM 的许可证保留在 `vendor/SOEM/LICENSE.md`，本项目没有重新授权其源码。

## 4. 今天就可以自己做的操作

在 `D:\qianrus\EtherCAT\Project_L` 的 PowerShell 里运行：

```powershell
.\EtherCAT_Master_Lab\build.ps1
.\EtherCAT_Master_Lab\list-adapters.ps1
```

第一条配置并构建官方 `slaveinfo` 和 `simple_ng`。第二条不传网卡参数，只列出可用接口，不扫描网络。

找到以下描述及对应接口名称：

```text
\Device\NPF_{52528361-DC95-49EC-95CF-1D2653888A6F}
Realtek Gaming 2.5GbE Family Controller
```

这是当前电脑实际枚举到的有线接口。网卡 GUID 将来可能因驱动安装或换机改变，到时以重新枚举的结果为准。Windows 的“以太网”显示名称不是这个示例所需的 pcap 接口名称。

同时出现 Wi-Fi、VMware、TAP、Loopback 等接口是正常的。选择物理 Realtek 有线接口作为实验链路。

看到接口意味着程序成功加载 pcap 组件并列出设备，不等于已经验证 EtherCAT 收发与从站通信。

## 5. 最少购买什么

已有 H743 最小板和 ST-LINK，优先复用。先购买一块完整的 LAN9252 SPI 从站模块和一根网线。

模块应包含网口及其配套电路、供电电路、时钟与 EEPROM，提供 SPI PDI 接口、原理图、从站源码和匹配的 ESI XML。确认 IRQ、SYNC0 引脚可以引出，便于后续学习中断和同步。确认供电与 SPI I/O 电平，H743 端按实际原理图连接。

给卖家的问题：

> 我用 STM32H743 最小板通过 SPI 开发 EtherCAT 从站。这个模块是否已配置为 SPI PDI？包含 EEPROM 吗，出厂是否已初始化？能否提供原理图、EEPROM 初始化方法、完整可编译的从站示例和匹配 ESI？示例协议栈来自哪里，是否支持 CoE、SDO 和 PDO？IRQ、SYNC0 能否引出？电源输入和 SPI 电平分别是多少？

不能只买 LAN9252 裸芯片；只标“EtherCAT IO”但不能提供 MCU 二次开发接口与源码的模块，也不能直接用于本项目的 H743 从站开发。

只有 F407 示例时，需要迁移 SPI、GPIO、中断和协议栈硬件访问层，不能直接把 F407 固件烧进 H743。拿到资料后再确定接线与移植。若 F407 + LAN9252 一体板总价只比完整模块贵一点，且源码已验证，一体板也是省时间的选择。

LAN9252 只承担 ESC 工作，H743 仍需要运行从站协议栈。我们当前纯 C 模拟器没有真实网络、ESC/PDI 或完整协议栈，不能直接被 SOEM 扫描出来。

## 6. 从站到手以后按顺序验证

1. 先跑卖家的原始示例。准备匹配的固件与 EEPROM 内容，确认从站能够启动。
2. 给从站按要求供电，电脑有线口直连 EtherCAT IN，检查链路灯。
3. 重新枚举接口，在有权限访问 Npcap 的 PowerShell 中执行扫描。若遇到权限错误，再使用管理员 PowerShell。

```powershell
.\EtherCAT_Master_Lab\scan-slaves.ps1 -Interface '\Device\NPF_{52528361-DC95-49EC-95CF-1D2653888A6F}'
```

脚本按当前官方示例的命令格式，把接口作为第一个参数传入，不加 `-i`。从站发现后，应看到非零从站数量，并能核对厂商 ID、产品码及状态。

读取 SDO 信息和查看映射可分别执行：

```powershell
.\EtherCAT_Master_Lab\scan-slaves.ps1 -Interface '\Device\NPF_{52528361-DC95-49EC-95CF-1D2653888A6F}' -Details Sdo
.\EtherCAT_Master_Lab\scan-slaves.ps1 -Interface '\Device\NPF_{52528361-DC95-49EC-95CF-1D2653888A6F}' -Details Mapping
```

这些功能依赖从站实际支持的 CoE 和对象字典。官方示例即使没找到从站，也可能返回退出码 0，所以要看从站数量和输出内容。

4. 再读一个已知对象，例如从站支持的 0x1000 Device Type。先确认返回值及字节数，随后学习单个 SDO Upload/Download API。
5. 根据这个从站的真实 PDO 布局，写一个最小周期主站程序。先传按键、LED或模拟位置，用数值变化验证双向通信。
6. 检查状态转换、预期 WKC、超时处理；初期按较宽松周期调试，并让从站 watchdog 配置与主站调试节奏匹配。
7. 加入 CiA402、模拟关节与故障处理，再测量循环周期及抖动。普通 Windows 下设置 Sleep(1) 或请求 1 ms，并不能证明满足严格 1 ms 实时要求。

`slaveinfo` 不是完整的持续运行控制主站；`simple_ng` 是官方周期通信参考程序，学习代码后还需要按实际从站映射和应用要求修改。SOEM 常见动态发现路径会读取从站 EEPROM/SII，不要求像 TwinCAT 那样先导入 ESI，但配套 ESI 和 PDO 说明仍是检查配置的重要资料。

## 7. Wireshark 如何辅助观察

从官方安装 Wireshark，选择 Realtek 有线接口抓包，显示过滤器填写：

```text
eth.type == 0x88a4
```

扫描时观察 EtherCAT 命令、地址和 WKC；周期通信时观察过程数据。抓到发出的帧只说明程序发包，仍需主站收到有效响应、发现从站并核对 WKC，才算链路通信通过。

## 8. 当前验收与下一课

已通过：官方源码下载、MinGW/CMake 构建、Npcap 服务检查、官方示例网卡枚举。

未进行：从站扫描、SDO/PDO、OP、持续循环与 1 ms 实时性测试，因为目前没有 EtherCAT 从站硬件。

现在可以自己运行构建和枚举命令，找到 Realtek 对应的接口，理解 SOEM 与 Npcap 的分工。然后继续原学习路线：在现有 `EtherCAT_Slave_Simulator` 中逐步学习对象字典与 SDO；接到实际硬件后，用主站读取这些对象来验证。

官方资料：

- [SOEM 源码](https://github.com/OpenEtherCATsociety/SOEM)
- [本次提交](https://github.com/OpenEtherCATsociety/SOEM/tree/88e8ed46efba7dfa7b94d08a512db25a33e3f8d5)
- [Npcap 下载与说明](https://npcap.com/)
- [Wireshark 下载](https://www.wireshark.org/download.html)
- [LAN9252 产品资料](https://www.microchip.com/en-us/product/LAN9252)
