# EtherCAT Slave Simulator：第六课

当前累计工程已加入[第七课：对象字典](Lesson_07_对象字典.md)和[第八课第一步：Mailbox 与 SDO 请求/响应](Lesson_08_Mailbox与SDO.md)。本文件保留第六课讲解和当时的基线输出；当前运行还会打印第七、八课演示。

当前学习[第八课第二步：SDO 字节格式](Lesson_08_第二步_SDO字节格式.md)，只增加快速 SDO 内容的编解码，尚未实现完整 CoE/Mailbox 封装或真实网络通信。

第六课目标：用标准 C 观察「应用变量 → PDO 字节 → 应用变量」。当时版本只有简化状态切换和固定 PDO 布局；累计代码已增加对象字典及 SDO 请求/响应语义模拟。尚无真实网络报文、ESC、完整 SDO 协议、CiA402 或电机模型。一次数据交换不代表已实现 1 ms 实时周期。

## 运行

在 Project_L 的 PowerShell 中执行（GCC 已在当前环境 PATH 中）：

```powershell
.\EtherCAT_Slave_Simulator\build.ps1
```

程序由 main.c、EtherCAT/ethercat_state.c、EtherCAT/ethercat_pdo.c、EtherCAT/ethercat_od.c、EtherCAT/ethercat_sdo.c、EtherCAT/ethercat_sdo_wire.c 编译而成，生成 build/ethercat_sim.exe。构建脚本也会运行程序。

## 1. 先分清方向

学习节奏调整：当前先达到「看懂数据流、能修改数值、能编译运行」即可，不要求脱离参考独立写出整个模块。第六课拆成小步，暂时只读 main.c 中的四步；底层负数转换、对象字典和映射配置留到理解当前例子以后。后续先仿照已有函数修改，再尝试自己补一个小函数。

RxPDO / TxPDO 都以从站为参照。主站的输出就是从站收到的 RxPDO；从站发送的 TxPDO 成为主站的输入。

```text
主站目标位置 → 主站输出过程映像 → 从站 RxPDO → 关节应用
主站实际位置 ← 主站输入过程映像 ← 从站 TxPDO ← 编码器反馈
```

Process Image（过程映像）是主站维护的过程数据缓冲区，给各个从站的数据安排位置。本课用 EC_ProcessImage 的 outputs[6] 和 inputs[6] 表示单关节的两个区域；实际主站可以用分离的输入/输出缓冲区，也可以组织统一逻辑地址空间。

## 2. 本课的固定布局

| 方向 | PDO 内字节偏移 | 对象 | 类型 | 长度 |
|---|---|---|---|---|
| RxPDO | 0–1 | 0x6040:00 Controlword 控制字 | uint16_t | 16 bit |
| RxPDO | 2–5 | 0x607A:00 Target Position 目标位置 | int32_t | 32 bit |
| TxPDO | 0–1 | 0x6041:00 Statusword 状态字 | uint16_t | 16 bit |
| TxPDO | 2–5 | 0x6064:00 Position Actual Value 实际位置 | int32_t | 32 bit |

每个方向是 16 + 32 = 48 bit，也就是 6 字节。位置暂用整数计数，物理单位和缩放留到关节控制课程。控制字、状态字的位含义留到 CiA402 课程；本课保持为 0，不能从这里判断驱动使能。

## 3. PDO Mapping 到底映射什么

对象字典给变量一个「索引 + 子索引」身份。PDO Mapping 规定从这些变量中选择哪些、按什么顺序和位宽放入 PDO。

假设我们的 RxPDO 使用映射对象 0x1600，TxPDO 使用 0x1A00，其内容应为：

```text
0x1600:00 = 2            有两个 RxPDO 映射项
0x1600:01 = 0x60400010   控制字，子索引 0，16 bit
0x1600:02 = 0x607A0020   目标位置，子索引 0，32 bit

0x1A00:00 = 2            有两个 TxPDO 映射项
0x1A00:01 = 0x60410010   状态字，子索引 0，16 bit
0x1A00:02 = 0x60640020   实际位置，子索引 0，32 bit
```

每个映射项是一个 32 位描述值：`(index << 16) | (subindex << 8) | bit_length`。这里末尾的 0x10 是 16 bit，0x20 是 32 bit。映射项描述字段，周期 PDO 数据中通常只传字段值，并不每次附带这些索引。

0x1600 和 0x1A00 是映射对象的索引，**不是 PDO 数据的内存地址**。上述表是我们选择的教学布局，并非所有驱动器的默认配置。本课通过打包函数固定实现这张表，第七课再引入对象字典。

PDO Assignment 又是另一层：常见配置中，0x1C12 把 RxPDO 分配给 SM2，0x1C13 把 TxPDO 分配给 SM3。例如 0x1C12:01 引用 0x1600；不是直接引用控制字 0x6040。具体 SM 分配依从站配置而定。

参考：[TI 官方过程数据配置说明](https://software-dl.ti.com/mcu-plus-sdk/esd/AM64X/08_06_00_43/exports/docs/industrial_protocol_docs/am64x/ethercat_slave/proc_data_config.html)；[Beckhoff 官方映射及分配对象示例](https://infosys.beckhoff.com/content/1033/el6080/2454368267.html)。

## 4. 在真实硬件上如何流动

配置关系：对象字典中的应用变量 → PDO Mapping 选择字段和布局 → PDO Assignment 分配给 SyncManager → SyncManager 管理 ESC 本地过程数据区 → FMMU 将逻辑地址映射到本地数据区。

运行时主站到电机的路径：

```text
主站应用填写输出过程映像
  → EtherCAT Datagram 携带过程数据
  → ESC 的 FMMU 匹配逻辑地址并映射本地地址
  → 写入由 SyncManager 管理的 DPRAM 区域
  → STM32 通过 PDI 读取有效 RxPDO 数据
  → 从站软件解包为控制字、目标位置
  → CiA402 检查状态和控制命令
  → 关节控制层 / 电机控制层执行
```

反馈方向：编码器 / 应用更新实际位置和状态字 → STM32 打包 TxPDO → PDI 写入 ESC 本地数据区 → ESC 将反馈放入主站读取的 Datagram → 主站更新输入过程映像。

ESC 主要处理报文、FMMU、SyncManager 和本地数据访问；应用变量和控制字的意义需要从站软件处理。EtherCAT OP 是驱动运行的通信条件；还需要 CiA402 Operation Enabled、合法模式及安全条件。

本课让主站和从站代码直接访问同一个 image，暂时省略传输与 ESC/PDI。不要把这个 C 数组理解为真实主站与 STM32 共享的一块物理内存。

## 5. 为什么结构体不能直接当 PDO 发出去

EC_RxPDO 是应用变量的容器。编译器可能在 uint16_t 和 int32_t 之间插入填充，使 sizeof(EC_RxPDO) 大于 6；本机字节序也不应成为协议布局的前提。因此打包函数逐字节写入规定的偏移，解包函数反向还原。

控制字 0，目标位置 1000 = 0x000003E8，采用小端顺序：

```text
字节偏移   0  1   2  3  4  5
RxPDO     00 00  E8 03 00 00
          控制字  目标位置
```

读代码时先看 main.c 中的四步，再看 EC_PackRxPDO / EC_UnpackRxPDO，最后看 ReadI32 / WriteI32。对负数的转换细节可以稍后细看，先理解字段宽度、偏移和小端顺序。

## 6. 基线输出

```text
EtherCAT Slave Simulator Start
State: INIT
INIT -> PRE-OP
PRE-OP -> SAFE-OP
SAFE-OP -> OP
EtherCAT Slave Operational
Master outputs / Slave RxPDO: 00 00 E8 03 00 00
Slave received: controlword=0x0000, target_position=1000
Slave TxPDO / Master inputs: 00 00 00 00 00 00
Master received: statusword=0x0000, actual_position=0
```

状态模块仅演示依次向上切换、保持当前状态和回到 INIT，并拒绝 INIT 直接跳 OP；不代表完整 EtherCAT 状态转换规则。配置检查、AL 错误、其他向下转换将在后续补充。

## 7. 留给你完成

1. 找到 main.c 中的 target_position = 1000，改成 2000。先写出预期的六个字节，再运行对照。
2. 找到 slave_feedback.actual_position = 0，改成 300，模拟编码器。观察主站能否收到 300，并解释目标位置和实际位置为何可以不同。
3. 思考：如果把 RxPDO 中的目标位置排在控制字前面，主站和从站哪几处必须一起改？只改结构体成员顺序是否足够？

我们先讨论这些结果，再继续对象字典；避免一次加入所有模块。

## 8. 练习参考答案

1. 2000 = 0x000007D0，目标位置的小端字节为 D0 07 00 00。控制字仍为 0，所以完整 RxPDO 是 `00 00 D0 07 00 00`，从站解包后打印 target_position=2000。
2. 300 = 0x0000012C，状态字仍为 0，所以 TxPDO 是 `00 00 2C 01 00 00`，主站解包后打印 actual_position=300。目标是期望到达的位置，实际是当前反馈位置，两者不要求相同；当前程序没有电机运动模型，不会自动追踪目标。
3. 若目标位置排在控制字前面，则目标位置占偏移 0–3，控制字占偏移 4–5；EC_PackRxPDO 和 EC_UnpackRxPDO 都要一起调整偏移。真实系统还要让 PDO Mapping 的字段顺序及主站配置保持一致。本课显式按成员名称和固定偏移读写，所以只交换结构体成员顺序不会改变传输布局；长度仍是 6 字节，TxPDO 布局无需随之改变。

修改后先保存源文件，再重新运行 build.ps1。单独运行旧的 exe 不会把源文件的修改编译进去。如果 PowerShell 提示执行策略禁止运行脚本，可以在项目根目录直接编译再运行：

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror .\EtherCAT_Slave_Simulator\main.c .\EtherCAT_Slave_Simulator\EtherCAT\ethercat_state.c .\EtherCAT_Slave_Simulator\EtherCAT\ethercat_pdo.c .\EtherCAT_Slave_Simulator\EtherCAT\ethercat_od.c .\EtherCAT_Slave_Simulator\EtherCAT\ethercat_sdo.c .\EtherCAT_Slave_Simulator\EtherCAT\ethercat_sdo_wire.c -o .\EtherCAT_Slave_Simulator\build\ethercat_sim.exe
```

确认编译成功后再执行：

```powershell
.\EtherCAT_Slave_Simulator\build\ethercat_sim.exe
```
