# 第八课第一步：Mailbox、CoE 与 SDO 请求/响应

第七课已完成，标签为 lesson-07。本课先理解请求怎么到达对象字典；不要求逐行吃透实现。保留你的练习：PDO 目标 2000、实际位置 300、字典写入 4000。

阅读代码前，可以先看[程序运行流程图](../../docs/程序运行流程图.md)：第一张图对应 main 的顺序，第二张图展开 EC_SDO_Transfer 的发送、处理和接收。

这是第一小节说明，阶段版本为 lesson-08-part1。当前已进入[第二小节：SDO 字节格式](Lesson_08_第二步_SDO字节格式.md)，它在原模拟请求和邮箱之外增加字节编解码；下面对实现范围的描述对应第一小节版本。

## 1. 为什么有了字典还要 SDO

第七课可以直接调用 C 函数读写从站变量，真实主站和从站却在不同设备上，需要交换请求与响应。

| 名称 | 本课先理解什么 |
|---|---|
| 对象字典 OD | 从站中有哪些对象，编号、类型和访问权限是什么 |
| SDO | 按索引和子索引，请求读写对象的服务 |
| CoE | CANopen over EtherCAT，承载 CANopen 对象与服务的 EtherCAT 应用协议 |
| Mailbox | 用于交换这类消息的通信机制，可承载 CoE 等协议 |

CoE 使用 EtherCAT，不需要额外建立一条 CAN 总线。SDO 是 Service Data Object。

参考：[Beckhoff Mailbox CoE 说明](https://infosys.beckhoff.com/content/1033/tc3_io_intro/1357984011.html)、[CoE 对象访问及启动下载请求](https://infosys.beckhoff.com/content/1033/el252x/1037003019.html)。

## 2. Upload 和 Download 的方向

| 服务 | 主站要做的事 | 主要数值流向 |
|---|---|---|
| SDO Upload | 读取从站对象 | 从站 → 主站 |
| SDO Download | 写入从站对象 | 主站 → 从站 |

Upload 时主站仍要先发送读取请求，从站再返回数值；名称指对象数据的传输方向，不表示只有从站主动发消息。Download 时从站也要返回确认或失败响应。

## 3. 一条读取请求包含什么

主站想读目标位置，语义上要表达：

```text
服务：Upload，读取
索引：0x607A，目标位置
子索引：0
```

从站解析后，相当于执行第七课的：

```c
EC_OD_ReadI32(&od, 0x607A, 0, &value);
```

再把成功状态和读到的 value 放入响应。写请求则带上要写的数值，交给 EC_OD_WriteI32；查表和权限检查由原来的字典负责。

## 4. 真实系统的数据路径

```text
主站构造 SDO 请求
  → 封装在 CoE 消息中
  → 放入 EtherCAT Mailbox 消息
  → 主站通过 EtherCAT 访问从站邮箱区
  → ESC / SyncManager 管理邮箱数据区
  → MCU 通过 PDI 取出消息
  → 从站软件解析 CoE / SDO
  → 访问对象字典
  → 生成响应，经邮箱返回主站
```

常见从站用 SM0 管理主站到从站的邮箱，SM1 管理从站到主站的邮箱，实际分配依设备配置而定。这条邮箱路径与 SM2/SM3 的周期 PDO 路径分开管理。

SDO 常用于参数、启动配置和诊断，PDO 常用于周期目标与反馈。SDO 不限于启动时使用，也可以在设备运行时按需访问；具体对象是否允许写、在哪些状态允许写，要看设备定义。普通 CoE 邮箱通信通常可在 PRE-OP、SAFE-OP、OP 使用。

## 5. 本课如何模拟

先使用表示已解析内容的 C 结构体，不进行报文字节打包：

```c
EC_SDO_Request request = {
    .service = EC_SDO_DOWNLOAD,
    .index = 0x607A,
    .subindex = 0,
    .value = 5000
};
```

意思是：请求把从站目标位置写为 5000。这是程序内的请求描述，不能直接当 EtherCAT 报文发出去。

EC_Mailbox 用一个请求槽和一个响应槽模拟消息交换，依次经过：

```text
EMPTY → REQUEST_READY → RESPONSE_READY → EMPTY
空闲     请求待处理       响应可读取        本次结束
```

对应函数：Send 复制请求；Process 根据服务调用 OD_Read/OD_Write；Receive 复制响应并恢复空闲。未处理的请求和未取走的响应都不能被新请求覆盖。

EC_SDO_Transfer 把三步依次调用，方便第一遍看懂完整事务。它返回 true 表示收到了响应；还需要看 response.result，确认对象读写是否成功。只读写入被拒绝时，事务仍然能够正常返回失败响应。

本模型单线程、同步执行，只处理 int32_t；没有实现网络传输、Mailbox/CoE 头、SDO 命令字、标准 Abort Code、分段传输、超时重试或通信状态检查。response.result 暂时复用本地 OD 结果；这不是标准 SDO 报文实现。下一小步再解读和加入实际字节格式，不能把结构体内存布局直接当协议布局。

## 6. 运行与观察

```powershell
.\EtherCAT_Slave_Simulator\build.ps1
```

第六、七课输出之后新增。以下是 lesson-08-start 开始版本的 5000 示例：

```text
Lesson 8: SDO / Mailbox semantic simulator
SDO Upload 0x607A:00: value=4000
SDO Download 0x607A:00: OK, slave target_position=5000
SDO Upload 0x607A:00 after write: value=5000
SDO Download 0x6064:00: READ_ONLY, actual_position=300
```

4000 来自你第七课的练习；5000 是本课新写入值；实际位置 300 没变。

当前你已将 main 中的请求值改为 6000，Download 和随后 Upload 的两行数值都变为 6000。前面的 Upload 仍读到 4000，第六、七课输出不受影响；流程图已按当前练习数值更新。

这一演示复用了目标位置，便于观察两种访问路径使用同一变量。真实关节的周期目标通常由 PDO 更新；如果下一周期收到新的 RxPDO，它会再次覆盖目标位置。SDO 参数写入也不等于参数已保存到 Flash，持久化是另外的机制。

## 7. 只做一个练习

在 main.c 第八课部分，把 `request.value = 5000;` 改为 6000，保存、编译运行。先预测第八课哪些输出会变，第六、七课输出是否会变。

先看请求的四个字段及输出即可。读实现时先看 EC_Mailbox_Process 中的两个分支，找到它们调用第七课函数的位置，再看其他邮箱状态检查。
