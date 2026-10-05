# 第九课：PDO 怎样到达从站变量

第八课已完成并归档为 `lesson-08`。本课开始版本为 `lesson-09-start`，完课版本为 `lesson-09`。第 1–9 节保留开始版本的讲解与练习，当前成果和实际运行见第 11 节及累计流程图；第 10 节是另一任务整理的排错经验。

这一课先只解决一个问题：第六课的 6 字节 PDO，怎样经过 ESC 本地数据区到达从站应用？沿用原来的打包、解包函数，不增加新的 PDO 字段。

学习目标：能沿流程图解释 FMMU、SyncManager、DPRAM、PDI 各自的作用，并指出“哪一步真正更新了目标位置变量”。先读 main 中第九课的五步；不要求独立写底层模块。

## 1. 从已经会的部分接上来

第六课为了简化，直接这样做：

```text
主站 Pack → image.outputs → 从站 Unpack → slave_command.target_position
```

这次在中间补上地址和本地数据区：

```text
主站 Pack → 逻辑地址 → FMMU → SM 管理的本地数据区 → PDI 读 → Unpack
```

主站与从站各有自己的内存。真实通信中，主站不会把本机数组的指针传给 STM32；ESC 处理经过的报文，STM32 再通过本地接口获取数据。

## 2. 先理解四个名称

| 名称 | 作用 | 本课对应代码 |
|---|---|---|
| FMMU | 将逻辑地址范围映射到 ESC 本地地址范围 | EC_FMMU_Translate |
| SyncManager，简称 SM | 管理一段本地数据区的访问方向与一致的数据交接 | EC_SM_Write / EC_SM_Read |
| DPRAM | ESC 中用于交换数据的双端口 RAM | 每个 SM 的 bytes[6] 代表自己的数据区 |
| PDI | 从站处理器访问 ESC 的本地接口 | 调用时指定 EC_SM_PDI_SIDE |

FMMU 的基本换算是：

```text
本地地址 = 本地起点 +（请求的逻辑地址 − 逻辑起点）
```

换算之前要检查方向和范围。**地址换算本身不复制字节，不解包 PDO，也不访问对象字典。**

SM 则决定谁可以写、谁可以读，并交付完整的一份数据。真实 ESC 的过程数据 SM 通常使用三缓冲，读端获得一致的最新数据；邮箱型 SM 通常使用单缓冲和逐笔握手。两者用途不同：过程数据允许新值替换尚未读取的旧值，而邮箱要避免覆盖尚未取走的消息。

依据：[Beckhoff 官方 FMMU / SM 说明](https://infosys.beckhoff.com/content/1033/tc3_io_intro/4981170059.html)。本课只用顺序调用的整块复制表达“完整发布 → 读取最新完整值”，没有实现硬件三缓冲、并发访问或真实 PDI 驱动。

## 3. 一定要区分这几种数字

| 数字 | 含义 | 能不能互换？ |
|---|---|---|
| 0x607A:00 | 目标位置对象的索引和子索引 | 是对象身份，不是内存地址 |
| PDO 偏移 2 | 目标位置在本课 6 字节 PDO 中从第几个字节开始 | 来自固定 PDO 布局 |
| 逻辑地址 0x00000000 | 本例主站写命令数据的逻辑起点 | 由本例配置选择 |
| 本地地址 0x1000 | 本例从站命令数据区的起点 | 由本例配置选择，不是 C 指针 |

FMMU 不知道目标位置叫 0x607A。它只按配置换算地址；从站软件按 PDO 布局解包，才把目标值放入 slave_command.target_position。对象字典随后通过已有指针访问同一个变量。

## 4. 本课只配置两个方向

| 数据 | 主站逻辑区间 | ESC 本地区间 | 本例 SM | 谁写 → 谁读 |
|---|---|---|---|---|
| RxPDO 命令，6 字节 | 0x00000000–0x00000005 | 0x1000–0x1005 | SM2 | EtherCAT 侧 → PDI 侧 |
| TxPDO 反馈，6 字节 | 0x00000010–0x00000015 | 0x1010–0x1015 | SM3 | PDI 侧 → EtherCAT 侧 |

这些地址是教学选择，并非所有从站都必须用它们；SM2/SM3 是本例采用的常见分配。配置放在第九课演示开头；真实系统会在启动阶段配置 ESC，本课在已进入 OP 的累计示例末尾准备软件模型。

命令区的目标位置从 PDO 偏移 2 开始，因此：

```text
逻辑 0x00000002 → 本地 0x1002，随后连续 4 字节存目标位置
```

在本例中 FMMU 可以计算子区间地址，但 SM 接口只支持从起点完整复制 6 字节，不支持拆成几笔部分写入。先保证一次得到完整命令，避免混用旧控制字和新目标位置。

## 5. 程序运行流程图

前面的第六、七、八课依次演示完，再执行下面的第九课流程。主站和从站仍是同一程序中的模拟角色；每个函数都执行完成后才进入下一步。

```mermaid
flowchart TD
    A["准备 SM2 / SM3 与两条 FMMU 映射"]
    B["主站：目标 9000<br/>PackRxPDO → image.outputs"]
    C["FMMU：逻辑写 0x00000000<br/>换算到本地 0x1000"]
    D["SM2：EtherCAT 侧完整写入 6 字节"]
    E["从站：PDI 侧读取 SM2 → pdi_rx"]
    F["UnpackRxPDO<br/>原目标变量更新为 9000"]
    G["OD 读取同一个目标变量 → 9000"]
    H["从站：实际位置仍为 300<br/>PackTxPDO → pdi_tx"]
    I["SM3：PDI 侧完整写入反馈"]
    J["FMMU：逻辑读 0x00000010<br/>换算到本地 0x1010"]
    K["SM3：EtherCAT 侧读取 → image.inputs"]
    L["主站：UnpackTxPDO → 实际位置 300"]
    M["检查两种预期失败<br/>未映射地址、向 Rx 通道反向写入"]
    N(["检查符合预期后 return 0"])
    A --> B --> C --> D --> E --> F --> G --> H --> I --> J --> K --> L --> M --> N
```

正常流程中的函数返回值必须检查。成功路径之外的意外失败让 main 返回 1；最后的两次拒绝是故意演示，符合预期则正常结束。

完整累计流程见[程序运行流程图](../../docs/程序运行流程图.md)。

## 6. 编译运行并观察

修改后保存文件，在 Project_L 根目录执行：

```powershell
.\EtherCAT_Slave_Simulator\build.ps1
```

脚本重新编译再运行。前面第六至八课的输出仍保留，末尾新增：

```text
Lesson 9: FMMU / SyncManager process data path
Mapped master outputs: 00 00 28 23 00 00
Rx mapping: logical=0x00000000 -> physical=0x1000, length=6
PDI RxPDO: slave target_position=9000
OD sees mapped target=9000
Tx mapping: logical=0x00000010 -> physical=0x1010, length=6
Mapped master inputs: 00 00 2C 01 00 00
Mapped feedback: actual_position=300
Unmapped logical 0x00000020: REJECTED
PDI write to Rx SM2: WRONG_DIRECTION, slave target_position=9000
```

9000 = 0x00002328，所以命令六字节是 `00 00 28 23 00 00`。FMMU 改变的是数据对应的地址，数据本身仍是第六课的小端布局。

REJECTED：逻辑地址 0x20 不属于本例 Rx 映射。WRONG_DIRECTION：Rx 通道的命令来自主站，PDI 侧在本模型中只能读它，不能反向写。两次检查均不改变已经收到的目标值。

SM 尚未收到第一份完整数据时读取会得到 NO_DATA；新数据发布后可重复读取。若写入比读取快，读到最近的一份完整数据，不排队保留所有旧周期。

## 7. 先读这些代码就够了

1. main.c 的「第九课从这里开始」：对照五步和图看运行顺序。
2. ethercat_fmmu.h 的 EC_FMMU：看逻辑起点、本地起点、长度、读写允许。
3. ethercat_fmmu.c 的最后一句换算：理解偏移如何保持一致。
4. ethercat_sm.h：理解 EtherCAT/PDI 两侧在 Rx 与 Tx 中互换生产者角色。

能说明每个调用拿什么、输出什么之后，再读 sm.c 的检查和复制。C 语法辅助：`&physical_address` 是把地址交给函数填写换算结果；`sm->bytes` 是通过指针访问通道里的数组；memmove 复制指定数量的字节，没有“识别目标位置”的能力。

完整底层实现暂时不用背，也不要求你现在独立仿写。测试文件 tests/verify_lesson09.c 是验证用，课堂可先跳过。

## 8. 一个必做的小练习

只把第九课的 `master_command.target_position = 9000` 改成 10000。前面各课的数值不动。预测命令六字节、OD 读数和实际反馈，再保存、编译运行。

参考答案：10000 = 0x00002710，六字节是 `00 00 10 27 00 00`。从站目标和 OD 读数变为 10000，逻辑及本地地址保持原配置，实际位置仍为 300。修改目标不会让这个模拟器产生电机运动。

可选地址练习：把 **SM2 初始化起点**和 **rx_fmmu.physical_start** 同时从 0x1000 改成 0x1020，再运行。预测：Rx 输出地址变成 0x1020，命令数值不变，反馈仍用 SM3 的 0x1010。只改 FMMU 而不改 SM2，会因本地区间不匹配被拒绝，main 在该处返回 1；这不是数据值的问题。

## 9. 思考题与参考答案

- 命令六字节已经写进 SM2 数据区，目标变量是不是立即改变？没有。当前模型还需要 PDI 读、Unpack，把字节还原到目标变量。
- 0x607A 和 0x1000 有什么关系？前者是对象身份，后者是本例本地地址。关联来自 PDO 布局及应用解包，不是数值相等或直接相加。
- 为什么 Tx 通道是 PDI 写、EtherCAT 读？反馈由本地应用生产，主站消费；Rx 命令方向正好相反。
- 先写两次，再读一次，读到第一次还是第二次？本课过程数据模型读到第二份完整值；它表达最新值，不表达邮箱消息队列。

本课先达到「能解释地址映射和两侧读写方向，并跑通小练习」即可。硬件寄存器、真实三缓冲、Mailbox SM0/SM1、位级映射、网络报文及实时循环尚未实现；后续分小步展开。

## 10. 地址修改与排错经验（2026-10-03）

本节记录学习者修改地址后遇到的 `Simulator failed.`，以及根据当时源码分析得到的原因。前面的地址表和运行输出保留开始版本；本节单独说明修改后的对应关系。整理本节时课程仍在学习中，后续完课记录见第 11 节。

### 10.1 先用一句话串起六个概念

**主站使用逻辑地址访问过程数据；从站里的 ESC 根据 FMMU 配置，把这段逻辑地址对应到 ESC 内部的物理地址。**

| 名称 | 可以怎样理解 |
|---|---|
| 主站 | 发命令、收反馈的控制方，例如 PC |
| 从站 | 被主站控制的设备，例如整个关节控制器，内部可以包含 ESC 和 STM32 |
| ESC | 从站内部负责 EtherCAT 硬件通信的控制器，例如 LAN9252 |
| 逻辑地址 | 主站访问过程数据时使用的统一地址，可分配给不同从站 |
| 物理地址 | 这里指某个 ESC 内部的本地地址，不是 STM32 的 C 指针地址 |
| FMMU | ESC 内部的映射单元，将一段逻辑地址对应到一段物理地址 |

```text
主站请求写逻辑地址 0x20
          ↓ EtherCAT 网线
从站内部的 ESC
    FMMU：逻辑 0x20 → 本地物理 0x1020
          ↓
    SM 管理的本地数据区：保存命令字节
          ↓ PDI，例如 SPI
    STM32 读取、解包，再交给应用处理
```

真实硬件上 FMMU 在 ESC 内部；本课用 `EC_FMMU_Translate` 模拟地址检查和换算，主站与从站也只是同一个程序里的两种角色。

### 10.2 遇到了什么现象？

学习者先将 SM2 起点和 `rx_fmmu.physical_start` 配套改为 `0x1020`，随后尝试把 Rx FMMU 的逻辑起点从 `0x00000000` 改为 `0x00000020`：

```c
/* 这里只列出此次修改涉及的两个配置字段。 */
.logical_start = 0x00000020,
.physical_start = 0x1020,
```

运行构建脚本后，PowerShell 在 `build.ps1` 第 30 行报告：

```text
Simulator failed.
```

这表示编译已通过，但模拟器返回了非零退出码。脚本第 30 行是发现运行失败的位置；要继续回到 C 程序，查哪一处 `return 1` 被触发。

### 10.3 根因：映射规则改了，请求地址仍是旧值

PDO 长度是 6 字节，因此新的映射范围是：

```text
逻辑地址 0x20～0x25 → 本地物理地址 0x1020～0x1025
```

但 main 的第 2 步仍请求从逻辑地址 `0x00000000` 写入 6 字节：

```c
EC_FMMU_Translate(&rx_fmmu, EC_FMMU_WRITE, 0x00000000,
                  EC_RXPDO_SIZE, &physical_address)
```

第三个参数是**本次请求的逻辑地址**，不会随 `.logical_start` 自动改变。在 `ethercat_fmmu.c` 中：

```c
logical_address < fmmu->logical_start
```

此时 `0x00 < 0x20` 成立，函数返回 `false`。失败传递过程如下：

```text
主站请求范围不属于 FMMU 映射
    ↓
EC_FMMU_Translate 返回 false
    ↓
main 中 !false 为 true，进入 if，return 1
    ↓
PowerShell 的 $LASTEXITCODE 为 1
    ↓
build.ps1 抛出 Simulator failed.
```

该处条件使用 `||`。左边已经为真时，右边不会继续执行，所以这次 `EC_SM_Write` 尚未被调用，也没有将这份命令写入 SM2 数据区。

**结论：`0x20` 可以作为逻辑起点；失败来自配置与请求不一致，不是这个地址本身不能使用。**

### 10.4 如果要把逻辑起点迁移到 0x20，哪些位置要一起改？

| 位置 | 配套修改 |
|---|---|
| Rx FMMU 配置 | `.logical_start = 0x00000020` |
| 第 2 步正常写请求 | `EC_FMMU_Translate` 的第三个参数改为 `0x00000020` |
| 第 2 步运行提示 | `Rx mapping` 的文字改为 `logical=0x00000020` |
| 第 5 步未映射地址演示 | 请求地址从 `0x00000020` 改为映射外的地址，例如 `0x00000040` |
| 第 5 步运行提示 | 同步改为 `Unmapped logical 0x00000040` |
| 相关中文注释 | 同步说明新的逻辑起点与本地起点，避免注释仍写旧地址 |

本地起点继续使用 `0x1020` 时，SM2 初始化起点与 `rx_fmmu.physical_start` 都保持 `0x1020`。Tx 反馈映射不必随 Rx 一起移动。

**容易漏掉的第二个问题：第 5 步原来故意用 0x20 测试“未映射”。** 如果正常写请求已改为 0x20，但这项失败演示没有改，0x20 现在属于有效映射，`matched` 就会为 `true`，输出 `MATCHED`。末尾的检查仍会失败：

```c
if (matched || wrong_side != EC_SM_WRONG_DIRECTION) {
    return 1;
}
```

这里失败是因为“本应被拒绝的测试请求现在被允许了”。要保留原练习意图，就将这项请求改为范围外的地址，例如 0x40；PDI 侧反向写入 Rx 通道的拒绝检查仍保留。

配套修改后，根据源码应得到以下地址检查结果；这是预期结果，不是本次新增的实测运行记录：

```text
Rx mapping: logical=0x00000020 -> physical=0x1020, length=6
Unmapped logical 0x00000040: REJECTED
```

### 10.5 当前保存的练习与复盘结论

整理本节时，只读核对到 main 已保存：第九课目标为 `10000`，SM2 与 Rx FMMU 本地起点为 `0x1020`，Rx 逻辑起点和正常请求地址均为 `0x00000000`，第 5 步仍检查 `0x00000020`。本次只补充文档，没有修改这些数值，也未把逻辑起点迁移到 0x20 记录为已完成练习。

以后改地址时，沿两段关系检查：

```text
主站请求的整个逻辑范围 → 必须被 FMMU 映射覆盖
FMMU 换算出的本地范围 → 必须符合 SM 的数据区配置
```

地址配置改变后，正常请求、故意失败的测试、输出提示和注释都要重新核对。改变地址对应关系不会改变 PDO 字节布局，也不会改变目标位置数值。

## 11. 完课记录（2026-10-03）

学习者确认进入下一课。本任务在错误经验记录任务完成后统一归档，保留第 10 节分析和学习者源码修改：目标位置 10000，SM2 与 Rx FMMU 的本地起点 0x1020，Rx 逻辑起点与正常请求仍为 0。

本次重新严格编译运行通过，关键结果如下；反馈仍为 300：

```text
Mapped master outputs: 00 00 10 27 00 00
Rx mapping: logical=0x00000000 -> physical=0x1020, length=6
PDI RxPDO: slave target_position=10000
OD sees mapped target=10000
Unmapped logical 0x00000020: REJECTED
PDI write to Rx SM2: WRONG_DIRECTION, slave target_position=10000
```

当前顺序是：主站打包 10000 → 逻辑 0 换算本地 0x1020 → SM2 发布 → PDI 读取 → 解包更新目标 → OD 读取 10000 → 反馈 300 经 SM3 返回 → 两次预期拒绝。源码注释和[累计程序流程图](../../docs/程序运行流程图.md)已同步此结果。

未将 Rx 逻辑起点迁移到 0x20 记录为已完成练习。下一课为 CiA402 基础：控制字与状态字。

## 验证记录与可选复现

开始版本已通过 GCC C11 严格警告编译运行、Windows PowerShell 5.1 / PowerShell 7 构建，以及独立边界验证。验证源码保存在 tests/verify_lesson09.c，覆盖地址偏移、越界和溢出、方向、首次发布、最新完整值和失败不修改数据。

若要复现验证，先运行上面的 build.ps1 创建 build 目录，再在项目根目录执行以下命令；课堂学习不要求运行它：

```powershell
$verificationArguments = @(
    '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror'
    'EtherCAT_Slave_Simulator/tests/verify_lesson09.c'
    'EtherCAT_Slave_Simulator/EtherCAT/ethercat_sm.c'
    'EtherCAT_Slave_Simulator/EtherCAT/ethercat_fmmu.c'
    '-o', 'EtherCAT_Slave_Simulator/build/verify_lesson09.exe'
)
& gcc @verificationArguments
if ($LASTEXITCODE -ne 0) { throw 'Verification compilation failed' }
& .\EtherCAT_Slave_Simulator\build\verify_lesson09.exe
if ($LASTEXITCODE -ne 0) { throw 'Verification failed' }
```
