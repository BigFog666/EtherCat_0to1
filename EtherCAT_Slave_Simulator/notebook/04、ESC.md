# 四、ESC——EtherCAT 从站的大脑

一、典型结构：

```
                 EtherCAT Network
                       │
                       ▼
                ┌─────────────┐
                │ PHY 物理层收发器│
                └──────┬──────┘
                       │
                       ▼
                ┌─────────────┐
                │     ESC     │
                │             │
                │ EtherCAT    │
                │ Processing  │
                │             │
                │ ┌─────────┐ │
                │ │ DPRAM   │ │
                │ ├─────────┤ │
                │ │ FMMU    │ │
                │ ├─────────┤ │
                │ │ SyncMgr │ │
                │ ├─────────┤ │
                │ │ Mailbox │ │
                │ └─────────┘ │
                └──────┬──────┘
                       │
                       │ PDI
                       ▼
                    STM32
                       │
                       ▼
                 Motor Control
```

二、ESC有类似mcu的DMA、Timer、ADC的硬件模块

```
ESC
 │
 ├── EtherCAT Frame Processing
 ├── Address Processing
 ├── FMMU
 ├── SyncManager
 ├── Mailbox
 ├── PDO
 ├── WKC
 └── EtherCAT State Machine
```

三、内部最重要的四个东西

```
FMMU       → 地址映射
SyncManager → 数据通道/同步
DPRAM      → 数据存储
Mailbox    → 非周期通信
```

四、DPRAM：**Dual-Port RAM** 双口 RAM。

这是 ESC 和 MCU 之间非常重要的一块共享数据区。

五、FMMU：**ieldbus Memory Management Unit**

可以把它理解成：**EtherCAT 地址翻译器。**

六、SyncManager：它负责管理 ESC 中不同的数据区域和访问方向。

可以先理解成：**数据通道管理器。**

RxPDO / TxPDO其中Rx / Tx 是站在 Slave 角度看的数据。

七、Mailbox：另一条数据通道。

它主要用于：**非周期、配置、参数、诊断等数据。**

八、PDO 和 Mailbox 的区别

```
                    EtherCAT
                       │
                       ▼
                      ESC
                 ┌─────┴─────┐
                 │           │
           Process Data    Mailbox
                 │           │
           PDO（跑实时控制）  SDO（改参数）
                 │           │
            SyncManager     CoE
                 │
                FMMU
                 │
               DPRAM
                 │
                PDI
                 │
               STM32
```

九、前四课总结

```
                         EtherCAT
                            │
                            ▼
                           PHY
                            │
                            ▼
                    ┌──────────────┐
                    │     ESC      │
                    │              │
                    │   FMMU       │◄──── Logical Address
                    │              │
                    │ SyncManager  │
                    │      │       │
                    │      ▼       │
                    │    DPRAM     │
                    │      │       │
                    │   Mailbox    │
                    └──────┬───────┘
                           │
                          PDI
                           │
                           ▼
                         STM32
                           │
              ┌────────────┼────────────┐
              ▼            ▼            ▼
           Encoder      CiA402       Safety
                           │
                           ▼
                     Motor Control
                           │
                           ▼
                          Motor
```

