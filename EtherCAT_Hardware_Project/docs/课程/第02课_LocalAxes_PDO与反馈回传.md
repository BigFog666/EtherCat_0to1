# 第 02 课：LocalAxes、PDO 与反馈回传

课程编号 H02，建立日期 2026-10-06。先回答第一课留下的两个问题，再沿实际代码追踪一份命令和一份反馈。本课读取当前源码，不执行主站、烧录或 EEPROM 写入。行号以当前文件为准，也可搜索给出的函数名定位。

## 1. 本课路线与目标

今天主要打开 `firmware/ssc_bridge.c`、`common/pdo.c` 和 `master/joint_master.c`。生成的 SSC 文件只读几个定位点，不要求从头读完整协议栈。

学完后应能解释：LocalAxes 是谁的数据、12 字节每一段放什么、反馈怎样从模型经过 ESC 到达电脑，以及 PDO 和 SDO 为什么能观察对应的状态。

## 2. LocalAxes 究竟是什么

打开 [生成的 cia402appl.c](../../firmware/generated/cia402appl.c)，定位第 66 行：

```c
TCiA402Axis LocalAxes[MAX_AXES];
```

这是商家 SSC 的 CiA402 示例中已有的轴数据数组。`TCiA402Axis` 是每个元素的类型，`LocalAxes` 是数组名，`MAX_AXES` 是数组容量配置。当前项目使用第一个轴，也就是 `LocalAxes[0]`；下标 0 表示第一个元素。

再打开 [生成的 cia402appl.h](../../firmware/generated/cia402appl.h)，定位第 612 行的结构体，重点看第 627～628 行：

```c
CiA402Objects Objects;
TOBJECT OBJMEM * ObjDic;
```

`Objects` 保存这一轴的对象值，例如控制字、状态字、实际位置。`ObjDic` 指向对象字典描述条目，其中包含对象索引、类型与数据地址等信息。二者通过初始化时设置的数据指针关联起来。

因此这句：

```c
LocalAxes[0].Objects.objPositionActualValue
```

从左到右读作：“第一个轴 → 轴的对象值集合 → 实际位置字段”。`.` 表示访问结构体成员。

### 为什么我们的模型要往这里写

打开 [ssc_bridge.c](../../firmware/ssc_bridge.c) 第 120～132 行，重点看：

```c
joint_update(&model, TIM5->CNT, bEcatOutputUpdateRunning != 0);
/* 更新原 SSC 字典变量，SDO Upload 与 PDO 反馈可观察同一份数据。 */
key = lock_irq();
published_feedback = model.feedback;
LocalAxes[0].Objects.objControlWord = model.command.controlword;
LocalAxes[0].Objects.objTargetPosition = model.command.target_position;
LocalAxes[0].Objects.objTargetVelocity = model.command.target_velocity;
LocalAxes[0].Objects.objModesOfOperation = model.command.mode;
LocalAxes[0].Objects.objStatusWord = model.feedback.statusword;
LocalAxes[0].Objects.objPositionActualValue = model.feedback.position;
LocalAxes[0].Objects.objVelocityActualValue = model.feedback.velocity;
LocalAxes[0].Objects.objModesOfOperationDisplay = model.feedback.mode;
LocalAxes[0].Objects.objErrorCode = model.error_code;
unlock_irq(key);
```

业务计算发生在 `joint_update`，结果在 `model.feedback`。随后桥接把结果发布到两个地方：

| 发布目标 | 本项目怎样使用它 |
|---|---|
| `published_feedback` | 周期 PDO 输入映射取得快照并编码 |
| `LocalAxes[0].Objects` | SSC 对象字典的对象值，可供相应 SDO 读取 |

例如模型计算出位置 300，就把 300 同时复制到反馈缓存和对象字典位置字段。两条路径发布对应的值，但独立发生的 PDO 与 SDO 读取可能跨越不同更新时间，不保证每次独立读数都相等。

当前适配还把生成文件第 395 行的 `CiA402_Application` 改成空实现，避免原 SSC 示例在 ISR 中再运行一套轴模型。实际状态与运动以我们的 `model` 为准；LocalAxes 主要保留 SSC 的轴对象及字典接入。适配来源保存于准备脚本，不能把当前整套架构说成官方原样示例。

### 对象索引怎样找到这个字段

看生成的 `cia402appl.c` 第 204～205 行：

```c
case 0x6064:
    pDiCEntry->pVarPtr = &LocalAxes[AxisCnt].Objects.objPositionActualValue;
```

`&` 是取地址。这里把对象 0x6064 的数据指针指向某一轴的实际位置字段。SSC 处理相应 SDO 读取时，可以通过字典找到该字段；这句只是建立关联，并没有立即向网口发数据。

## 3. 反馈到底怎样回到电脑

先看这条路径：

```text
joint_update 计算 model.feedback
→ Project_Poll 发布到 published_feedback
→ SSC 调用 APPL_InputMapping
→ Project_InputMapping 复制反馈并编码为 12 字节
→ SSC 把字节写入 LAN9252 的输入过程数据区
→ 主站发起周期交换，读取该过程数据
→ SOEM 更新电脑的 inputs 缓存
→ project_decode_feedback 得到 JointFeedback f
```

这里“输入/输出”按主站的过程映像方向命名：主站输出是发给从站的命令，主站输入是从从站读回的反馈。因此 `Project_InputMapping` 是板端准备返回电脑的数据，`Project_OutputMapping` 是板端接收电脑下发的数据。

### 板端：从反馈结构体变成字节

看桥接第 97 行：

```c
void Project_InputMapping(unsigned short *data)
{
    JointFeedback snapshot;
    uint32_t key = lock_irq();
    snapshot = published_feedback;
    unlock_irq(key);
    project_encode_feedback((uint8_t *)data, &snapshot);
}
```

`data` 指向 SSC 提供的过程数据缓冲区。函数先复制完整反馈，再用编码函数写入该缓冲区。`(uint8_t *)data` 让编码函数按字节访问，而不是按两个字节的 `unsigned short` 访问；`&snapshot` 传递这份反馈的地址。

再看生成的 `cia402appl.c` 第 793 行：

```c
void APPL_InputMapping(UINT16* pData)
{
    Project_InputMapping(pData);
}
```

这层把 SSC 的应用回调转给我们的桥接。随后打开 [生成的 ecatappl.c](../../firmware/generated/ecatappl.c)，看第 179 行：

```c
void PDO_InputMapping(void)
{
    APPL_InputMapping((UINT16*)aPdInputData);
    HW_EscWriteIsr(((MEM_ADDR *) aPdInputData), nEscAddrInputData, nPdInputSize);
}
```

第一句准备内存缓冲区 `aPdInputData`；第二句通过硬件端口把缓冲区写入 ESC。`nEscAddrInputData` 是输入过程数据的 ESC 地址，`nPdInputSize` 是长度，当前为 12。具体 SPI 读写下一阶段再展开。

所以 `published_feedback = model.feedback` 本身只是内存赋值，`Project_InputMapping` 本身主要是编码；真正的 ESC 写入在后面的硬件端口调用。LAN9252 处理 EtherCAT 帧，STM32 不需要在这里自己实现整个网口收发协议。

### 电脑：取得字节再解码

打开 [joint_master.c](../../master/joint_master.c)，看第 76 行：

```c
static int exchange(void)
{ ecx_send_processdata(&context); return ecx_receive_processdata(&context,EC_TIMEOUTRET); }
```

它调用 SOEM 发送过程数据并收取返回帧，返回工作计数 WKC。演示循环第 125～133 行先编码命令、调用交换，再检查 WKC，最后执行：

```c
project_decode_feedback(context.slavelist[1].inputs, PROJECT_PDO_BYTES, &f);
```

`inputs` 是电脑中的输入过程映像指针；解码后，`f.position` 等字段供主站判断下一步动作或记录 CSV。`slavelist[1]` 是 SOEM 的第一个实际从站，和板端轴数组的 `[0]` 属于两套编号。

上述是数据依赖路径，各步骤不一定在同一帧、同一轮同步完成。主站读到的反馈可能属于此前已经发布的结果；第一课实测中发出目标后隔几轮看到位置变化，正是需要考虑这些更新环节。

## 4. 12 字节里面放什么

打开 [pdo.h](../../common/pdo.h) 第 5 行：`PROJECT_PDO_BYTES` 固定为 12。两方向当前各 12 字节，布局如下：

| 字节偏移 | 长度 | 命令：主站→从站 | 反馈：从站→主站 |
|---|---:|---|---|
| 0～1 | 2 | controlword 控制字 | statusword 状态字 |
| 2～5 | 4 | target_position 目标位置 | position 实际位置 |
| 6～9 | 4 | target_velocity 目标速度 | velocity 实际速度 |
| 10 | 1 | mode 请求模式 | mode 显示模式 |
| 11 | 1 | 填充字节，编码时为 0 | 填充字节，编码时为 0 |

这是本项目固定映射的布局，不是所有 EtherCAT 设备都必须使用 12 字节。字段顺序和类型需要与 ESI、对象字典的 PDO 映射、从站及主站保持一致。

## 5. 把一份命令手算成字节

打开 [pdo.c](../../common/pdo.c) 第 35 行：

```c
void project_encode_command(uint8_t *p, const JointCommand *c)
{
    write16(p,c->controlword); write32(p+2,c->target_position);
    write32(p+6,c->target_velocity); p[10] = (uint8_t)c->mode; p[11] = 0;
}
```

`p+2` 表示从缓冲区偏移 2 字节的位置开始；因为 `p` 是字节指针，增加 2 就跳过两个字节。

以控制字 0x000F、目标位置 1000、目标速度 0、模式 8 为例：

```text
控制字 0x000F        → 0F 00
位置 1000=0x000003E8 → E8 03 00 00
速度 0              → 00 00 00 00
模式 8              → 08
填充                → 00
完整命令            → 0F 00 E8 03 00 00 00 00 00 00 08 00
```

“小端”就是低位字节先放。例如 0x03E8 的低字节是 E8，高字节是 03。

再看第 20 行解码：

```c
if (!p || !c || n != PROJECT_PDO_BYTES) return 0;
c->controlword = read16(p); c->target_position = read32(p+2);
c->target_velocity = read32(p+6);
c->mode = p[10] <= 127 ? (int8_t)p[10] : (int8_t)(-1-(255-p[10]));
return 1;
```

它检查指针和长度，再按相同偏移还原字段。`read16` 将 `p[1]` 左移 8 位并与 `p[0]` 合并；`read32` 对四个字节做类似操作。

### 负数目标 -1000 怎样表示

32 位表示为 `0xFFFFFC18`，按小端顺序写作 `18 FC FF FF`。编码时，负数转换到 `uint32_t` 按模 2³²得到对应无符号值，再逐字节取出。解码第 10 行使用：

```c
return v <= INT32_MAX ? (int32_t)v : -1 - (int32_t)(UINT32_MAX - v);
```

读出的无符号值超过最大正数时，通过公式还原负数，避免依赖超范围无符号到有符号转换的实现行为。以 -1000 为例，`UINT32_MAX-v=999`，于是结果为 `-1-999=-1000`。

模式的那句条件表达式是同样思路的 8 位处理：0～127 直接作为非负数，更大的字节还原为负数。当前演示使用模式 8 和 9，均走前一个分支。

## 6. 再把一份反馈手算成字节

第 40 行 `project_encode_feedback` 与命令编码采用对应布局。假设某次已发布反馈为状态字 0x0027、位置 300、速度 500、模式 9：

```text
状态字 0x0027       → 27 00
位置 300=0x0000012C → 2C 01 00 00
速度 500=0x000001F4 → F4 01 00 00
模式 9             → 09
填充               → 00
完整反馈           → 27 00 2C 01 00 00 F4 01 00 00 09 00
```

这只是编解码教学样本，不声称板子此刻处在这些状态。电脑第 28 行的 `project_decode_feedback` 逆向还原这四个字段。

## 7. 为什么不用结构体直接发送

`JointCommand` 的成员长度相加为 11 字节，但 C 编译器可能插入对齐填充。当前字段排列在常见工具链中会使结构体长度大于 12；具体值由 ABI 决定。即使偶然等于网络长度，也不能据此认定内存布局匹配。

本项目显式编码保证：偏移固定、字节序固定、长度固定、填充字节明确。不要把 `data` 强制转换成 `JointCommand *` 后直接访问，也不要用 `sizeof(JointCommand)` 替代网络协议的 12。

解码函数检查长度不等于验证全部通信：主站还要检查 WKC，模型还要判断模式、控制字、限值与通信超时。板端入口传给解码器的长度是配置常量，实际过程数据长度匹配还依赖 SSC 配置与映射检查。

## 8. 本课练习与参考解释

到 [学习记录 H02 区](学习记录.md) 完成六题。先自己推演，再对照本节。

1. `[0]` 是第一个轴，`Objects` 是轴对象值集合，最后的字段是实际位置。当前实际位置由 `joint_update` 更新，不由 LocalAxes 自行产生。
2. -1000 命令：`0F 00 18 FC FF FF 00 00 00 00 08 00`。
3. 反馈：`27 00 2C 01 00 00 F4 01 00 00 09 00`。
4. 依次经过模型反馈、发布缓存、映射编码、SSC 缓冲区、硬件端口写 ESC、主站交换和 inputs 缓存、反馈解码。只做内存赋值与编码还不代表电脑已经收到。
5. 不会直接使用新值。本项目的 PDO 输入映射从 `published_feedback` 取数据，没有从 LocalAxes 逐字段打包；只改字典字段会让两条路径的值不同，之后主循环还可能用模型结果覆盖该字典字段。
6. 结构体存在 ABI、对齐与字节序问题，网络布局必须显式规定。长度检查不能替代 WKC 或模型业务检查。

本课通过标准：能指出两条反馈观察路径，独立按偏移编码正负数，解释哪个函数准备字节、哪个调用写 ESC、电脑在哪里解码。记录练习与疑问后再核对完课，不要求立刻修改通信布局。

下一课回到 `joint.c`，沿使能、CSP、CSV、停止和故障复位追踪模型的状态与运动。
