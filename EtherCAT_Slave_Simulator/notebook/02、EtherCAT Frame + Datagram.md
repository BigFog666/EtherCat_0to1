# 二、EtherCAT Ethernet Frame + Datagram

一、建立整体结构：这里最重要的是：**一个 Ethernet Frame 里面可以放一个或者多个 EtherCAT Datagram。**

```
┌─────────────────────────────────────────────┐
│ Ethernet Header                             │
│                                             │
│ Destination MAC                             │
│ Source MAC                                  │
│ EtherType = 0x88A4                          │
├─────────────────────────────────────────────┤
│ EtherCAT Header                             │
│                                             │
│ Length                                      │
│ Reserved                                    │
│ Type                                        │
├─────────────────────────────────────────────┤
│ EtherCAT Datagram                           │
│                                             │
│ Command                                     │
│ Address                                     │
│ Length                                      │
│ IRQ                                         │
│ Data                                        │
│ WKC                                         │
├─────────────────────────────────────────────┤
│ EtherCAT Datagram 2 ...                     │
├─────────────────────────────────────────────┤
│ Ethernet FCS                                │
└─────────────────────────────────────────────┘
```

二、Ethernet Header && EtherCAT Header  

Ethernet Header 后面就是 EtherCAT。

EtherCAT Frame 开始有一个 EtherCAT Header。

三、Datagram：一个典型 Datagram：

```
┌───────────────┐
│ Command       │ 1 byte
├───────────────┤
│ Address       │ 2/4 bytes
├───────────────┤
│ Length        │
├───────────────┤
│ IRQ           │ 暂不学
├───────────────┤
│ Data          │
├───────────────┤
│ WKC           │ 2 bytes
└───────────────┘
```

四、Command：Datagram 的第一个关键字段。

```
APRD    读取
APWR	写入			AP = Auto Increment Addressing
APRW	读写

FPRD	**
FPWR	**			FP = Configured Station Address
FPRW	**						固定站地址

BRD		**
BWR		**			B = Broadcast
BRW		**				广播

LRD		**
LWR		**			L = Logical Address
LRW		**					逻辑地址
```

五、Address：告诉 ESC **我要访问哪里。**由Command可知寻址模式

​	Length：Data 有多少字节。

​	Data    ：真正传输的数据。

六、WKC：**Working Counter ** 先简单理解为：这次 Datagram 到底有多少个 EtherCAT 从站实际参与了处理。

