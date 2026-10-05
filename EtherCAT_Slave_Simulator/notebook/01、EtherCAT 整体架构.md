# EtherCAT 整体架构

一、Master和Slave是主从关系，两者不断交换数据

二、EtherCAT 使用的是：**标准 Ethernet 物理层/帧传输基础**；但不是TCP一类

```
0x88A4
```

标识 EtherCAT 帧。

三、EtherCAT 最关键的特点：**On-the-Fly**，也就是“**边经过，边处理**”。这是 EtherCAT 高实时性的核心机制之一。

四、**ESC** = EtherCAT Slave Controller，从站控制器，一个专门负责 EtherCAT 实时通信的硬件。

五、PDI：Process Data Interface，过程数据接口，ESC ↔ MCU 的接口

可以是SPI、Parallel等PDI方式，用于与从站主控芯片进行通信。

六、PDO = 周期性过程数据。

```
Master → Slave

Target Position
Target Velocity
Target Torque
Control Word

Slave → Master：

Actual Position
Actual Velocity
Actual Torque
Status Word
```

七、SDO：参数/配置数据![image-20260929155338913](C:\Users\gaobo\AppData\Roaming\Typora\typora-user-images\image-20260929155338913.png)

八、CoE：CANopen over EtherCAT类似CANopen，有以下关系

```
EtherCAT
 │
 └── CoE
       │
       ├── PDO
       ├── SDO
       └── Object Dictionary
```

九、CiA402 定义了很多电机驱动相关内容。是一种标准，类比AUTOSAR

十、整个系统框图，各名词之间的关系

```
                 EtherCAT Master
                       │
                       │ Ethernet
                       ▼
                ┌──────────────┐
                │     ESC      │
                └──────┬───────┘
                       │ PDI
                       ▼
                    STM32
                       │
              ┌────────┴────────┐
              ▼                 ▼
            CoE              Motor Control
              │
        ┌─────┴─────┐
        ▼           ▼
       PDO         SDO
        │
        ▼
      CiA402
        │
        ▼
     Robot Joint
```

