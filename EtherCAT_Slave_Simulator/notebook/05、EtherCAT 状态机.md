# 五、EtherCAT 状态机

从站为什么一直停在 SAFE-OP，进不了 OP？

一、 EtherCAT 强制从站一步一步进入工作状态：

```
INIT       硬件准备
PRE-OP     参数配置
SAFE-OP    通信试运行
OP         正式运行
```

二、INIT：此时基本不能进行正常应用层数据通信。

```
ESC 初始化
STM32 初始化
EtherCAT 基础通信初始化
```

三、PRE-OP：Mailbox开始可用，可以进行：

```
CoE
SDO
参数配置
对象字典访问
```

但是没有   **PDO** ；Master 可以配置设备；但是还没有开始正常周期过程数据通信

四、SAFE-OP

Master 已经完成：

```
PDO Mapping
FMMU 配置
SyncManager 配置
过程数据区域配置
```

但是不应该直接驱动危险输出。理解为：

```
传感器数据：可以上传
电机输出：保持安全，不能控制
```

五、OP：允许完整 PDO 数据交换。

六、状态对比表

| 状态    | Mailbox / SDO | PDO                | 电机输出     |
| ------- | ------------- | ------------------ | ------------ |
| INIT    | ❌             | ❌                  | 禁止         |
| PRE-OP  | ✅             | ❌                  | 禁止         |
| SAFE-OP | ✅             | 部分周期数据已工作 | 保持安全     |
| OP      | ✅             | ✅                  | 可以正常控制 |

七、EtherCAT Master 通常负责请求状态切换，Slave回应。

八、 **为什么不能 INIT 直接跳 OP？** 因为：

```
PDO地址
长度
数据结构
映射关系
```

都还没有确定。收集的数据到底放在哪里？Slave 根本不知道。

九、最常见问题：SAFE-OP → OP 失败

PDO 配置错误

SyncManager 配置错误

Watchdog超时

