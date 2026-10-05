# 三、EtherCAT 寻址方式

一、第一种AP：Auto Increment Address：偏向 EtherCAT 网络初始化/发现阶段。还没初始化会给一个编号 0 、1、2。。

二、第二种FP：Configured Station Address：就是配置好的站地址。一般是 EtherCAT Master 在初始化阶段完成配置。0x1001。。

三、第三种L：Logical Address：把所有周期过程数据映射到一个统一的逻辑地址空间。Master 的应用程序可以把 EtherCAT 网络看成一块周期更新的过程数据区。

四、PDO Mapping 和 Logical Address

```
PDO Mapping
     ↓
决定哪些对象进入周期过程数据
     ↓
Process Data
     ↓
映射到 Logical Address
     ↓
LRW
     ↓
EtherCAT Frame
```

五、Process Image：过程映像 / 过程数据映像

六、课程压缩图

```
                    EtherCAT Master
                          │
                          │
                 ┌────────┴────────┐
                 │                 │
          初始化/发现            周期通信
                 │                 │
                 ▼                 ▼
          Auto Increment      Logical Address
                 │                 │
                 ▼                 ▼
        Configured Address       PDO
                                   │
                                   ▼
                                  LRW
                                   │
                                   ▼
                              EtherCAT Slave
                                   │
                                  FMMU
                                   │
                                   ▼
                                  ESC
                                   │
                                  PDI
                                   │
                                   ▼
                                STM32
                                   │
                                   ▼
                             电机/关节控制
```

