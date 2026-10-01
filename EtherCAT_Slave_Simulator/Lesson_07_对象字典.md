# 第七课：对象字典，先学会按编号访问变量

本课只增加两个位置对象。先看本文件的例子和 main.c 后半部分，不要求自己从零写完整字典。第六课的目标位置 2000、实际位置 300 保留。

## 1. 为什么需要对象字典

在 C 代码中，我们知道成员名，可以写 `slave_command.target_position`。但是外部主站需要按双方约定的对象编号访问数据，而不是依赖你的 C 成员名。

对象字典把「编号、类型、权限、实际变量」关联起来。本课就是一张两行的表：

| 索引 Index | 子索引 SubIndex | 含义 | 数据类型 | 对外访问权限 | 实际变量 |
|---|---|---|---|---|---|
| 0x607A | 0 | 目标位置 | INTEGER32 / int32_t | RW，可读写 | slave_command.target_position |
| 0x6064 | 0 | 实际位置 | INTEGER32 / int32_t | RO，只读 | slave_feedback.actual_position |

`0x607A:00` 是索引和子索引合写的形式，不是一个 C 内存地址。当前都是简单变量，子索引用 0；数组和记录的子索引组织以后再讲。

目标位置与实际位置的对象定义可对照厂家文档：[目标位置](https://doc-legacy.synapticon.com/software/41/documentation_html/object_htmls/607A/index.html)、[实际位置](https://doc-legacy.synapticon.com/software/41/documentation_html/object_htmls/6064/index.html)。这是厂家旧固件文档，用于核对本课的基本对象定义；真实设备仍应按对应固件手册配置。

## 2. 先看这三个调用

```c
int32_t value = 0;

/* 按对象编号读取目标位置，结果存到 value。 */
EC_OD_ReadI32(&od, 0x607A, 0, &value);

/* 按对象编号读取实际位置。 */
EC_OD_ReadI32(&od, 0x6064, 0, &value);

/* 按对象编号修改目标位置。 */
EC_OD_WriteI32(&od, 0x607A, 0, 2500);
```

这里 `&od` 指定使用哪张字典；`0x607A` 指定索引；`0` 指定子索引；读取时的 `&value` 指定把结果放在哪里。写入时的最后一个参数直接是要写的数值。本课函数名里的 I32 表示只处理 int32_t。

这些函数会返回结果。示例只突出参数含义，实际 main.c 检查了返回结果，读取成功后才使用输出值。

## 3. 最值得理解的一根指针

初始化时传入：

```c
EC_OD_Init(&od, &slave_command.target_position,
           &slave_feedback.actual_position);
```

字典记住现有变量的地址，因此两条访问路径最终到达同一个变量：

```text
RxPDO 解包 ──────────────────→ slave_command.target_position
对象字典 0x607A:00 → 保存的指针 ─→ slave_command.target_position
```

目标位置只存一份。这样通过 PDO 更新后，字典读取的是新值；通过字典写入后，原来的从站成员也改变。这里的函数名是我们自己设计的教学接口，不是标准要求的 API 名称。

两个原变量要在使用字典时仍然存在。本课它们都定义在 main 中，在程序结束前一直有效。

## 4. 只读为什么还能更新

RO 表示通过对外的字典写接口不允许修改。例如不能通过 `EC_OD_WriteI32` 把编码器反馈伪造为 999。

但设备自身仍要更新实际位置，因此第六课中的这一行仍合法：

```c
slave_feedback.actual_position = 300;
```

读写权限是软件接口检查，不是给这块 C 内存加硬件写保护。

## 5. 和 PDO、SDO 的关系

对象字典描述设备有哪些对象以及如何访问。PDO Mapping 选择其中哪些对象进入周期数据，并规定布局。

SDO 在下一课学习：它是携带对象索引、子索引和读写请求的协议机制，后续会调用字典访问逻辑。本课只做本地查表与读写，还没有 Mailbox、CoE 报文或 SDO。

当前 PDO 仍使用第六课的固定打包函数，还不会自动扫描对象字典生成布局。我们先建立对应关系，后续再逐步加入映射对象。

main.c 的字典写入发生在一次 PDO 演示完成之后，所以不会重新发送这次命令，也不会让电机运动。如果以后继续接收 RxPDO，新收到的目标位置会再次更新同一个变量。真实系统要根据运行模式和配置协调命令来源。

## 6. 运行结果

运行方法不变，仍在项目根目录执行：

```powershell
.\EtherCAT_Slave_Simulator\build.ps1
```

第六课输出之后，应新增：

```text
Lesson 7: Object Dictionary
OD read 0x607A:00: 2000
OD read 0x6064:00: 300
OD write 0x607A:00: slave target_position=2500
OD write 0x6064:00: READ_ONLY, actual_position=300
```

这四行分别验证：按编号读目标、按编号读反馈、通过字典修改原变量、拒绝写入只读对象。

## 7. 只做一个小练习

找到 main.c 中 `EC_OD_WriteI32(&od, 0x607A, 0, 2500)`，把 2500 改成 4000，保存并重新运行。

先预测：哪一行输出会变成 4000？前面的 RxPDO 为什么仍然是目标位置 2000？不需要改查表函数或增加新对象。

本课建议阅读顺序：本说明 → main.c 第七课部分 → ethercat_od.h 中的 EC_OD_Entry → ethercat_od.c 的初始化和读写函数。每次只读一小段。
