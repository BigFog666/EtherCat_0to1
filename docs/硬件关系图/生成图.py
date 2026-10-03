"""生成中文离线关系图；只使用 Python 标准库，不改变课程源码。"""
from pathlib import Path
import html
import json

ROOT = Path(__file__).resolve().parent
C = {'cmd':'#2563eb','feedback':'#0f8b79','config':'#8051be','clock':'#b07808','power':'#cf742a','safety':'#c64253','all':'#42566a'}
parts = []
def add(s): parts.append(s)
def esc(s): return html.escape(str(s), quote=True)
def text(x,y,s,size=19,color='#405368',weight=400):
    add(f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" font-weight="{weight}">{esc(s)}</text>')
def rect(x,y,w,h,fill='#ffffff',stroke='#d6e1eb',r=12):
    add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}" stroke="{stroke}"/>')
def wire(d,kind='all',label=None,lx=0,ly=0,both=False,dash=False,paths=None):
    membership = paths or ('cmd feedback config' if kind == 'all' else kind)
    add(f'<g class="wire" data-path="{membership}"><path d="{d}" fill="none" stroke="{C[kind]}" stroke-width="3" marker-end="url(#{kind})"'+(f' marker-start="url(#{kind}-start)"' if both else '')+(' stroke-dasharray="8 6"' if dash else '')+'/>' )
    if label: text(lx,ly,label,17,C[kind],600)
    add('</g>')
nodes = {}
def node(id,x,y,w,h,title,lines,paths,detail,fill='#fff'):
    nodes[id]={'title':title,'detail':detail,'paths':paths}
    add(f'<g class="node" id="node-{id}" data-id="{id}" data-path="{paths}" role="button" tabindex="0" aria-label="{esc(title)}"><title>{esc(detail)}</title>')
    rect(x,y,w,h,fill)
    text(x+16,y+29,title,21,'#172e45',650)
    for i,line in enumerate(lines): text(x+16,y+56+i*25,line,17)
    add('</g>')

add('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1600 1630" width="1600" height="1630" aria-labelledby="svg-title svg-desc" role="img">')
add('<title id="svg-title">EtherCAT 机器人单关节：硬件与软件关系总图</title><desc id="svg-desc">从左至右为 PC 主站、LAN9252 从站模块与 STM32，下面为电源、功率驱动、电机、传感器与调试工具。点击模块可在离线网页中查看说明。</desc><defs>')
for key,color in C.items():
    add(f'<marker id="{key}" markerUnits="userSpaceOnUse" markerWidth="12" markerHeight="12" viewBox="0 0 8 8" refX="7" refY="4" orient="auto"><path d="M0,0 L8,4 L0,8 z" fill="{color}"/></marker>')
    add(f'<marker id="{key}-start" markerUnits="userSpaceOnUse" markerWidth="12" markerHeight="12" viewBox="0 0 8 8" refX="1" refY="4" orient="auto"><path d="M8,0 L0,4 L8,8 z" fill="{color}"/></marker>')
add('</defs><style>text{font-family:"Microsoft YaHei","PingFang SC",sans-serif}.node{cursor:pointer}.node:focus rect{stroke:#2563eb;stroke-width:3}.node.selected rect{stroke:#2563eb;stroke-width:3}.dim{opacity:.16}</style>')
rect(0,0,1600,1630,'#f4f7fb','#f4f7fb',0)
text(40,47,'从电脑里的目标值，到关节的真实运动',30,'#172e45',700)
text(40,80,'上面看全局，下面看芯片内部。实体边框是硬件；内部白色框同时展示硬件功能与软件职责。',19)
for x,w,t,sub in [(40,245,'PC 主站','决定要去哪里'),(355,245,'ESC 从站控制器','交换通信字节'),(670,245,'STM32 控制器','解释命令并控制'),(985,245,'功率驱动','给电机提供能量'),(1300,260,'电机与关节','运动并反馈')]:
    rect(x,105,w,80,'#fff');text(x+15,136,t,22,'#172e45',650);text(x+15,164,sub,17)
for x in [285,600,915,1230]: wire(f'M{x+4},145 H{x+61}','cmd')

for x,w,t,sub in [(40,280,'① PC / 笔记本','已有电脑 · Windows 功能实验'),(390,500,'② LAN9252 完整从站模块','后续硬件 · ESC 与双 PHY 集成在芯片中'),(960,600,'③ STM32H743 最小板','已有板卡 · 后续运行真实从站固件')]:
    rect(x,215,w,650,'#eaf0f7','#c5d5e5',18);text(x+18,248,t,24,'#172e45',700);text(x+18,277,sub,17)
node('master',60,298,240,92,'主站应用程序',['目标值 / 状态请求','收到位置、速度、故障'], 'cmd feedback config','电脑端程序决定目标值并消费反馈。当前 SOEM 实验只完成构建与网卡枚举，尚未验证真实从站通信。')
node('soem',60,420,240,116,'SOEM + 过程映像',['主站协议库','outputs：发往从站','inputs：从站反馈'], 'cmd feedback config clock','SOEM 是运行在 PC 的 C 主站库。Process Image 是主站本机的字节缓冲；逻辑地址是总线配置的地址空间，不是把 PC 内存指针传给 STM32。')
node('npcap',60,567,240,91,'Npcap / 网卡驱动',['访问二层以太网帧','软件组件'], 'cmd feedback config','Npcap 给 Windows 程序提供抓取和发送以太网帧的接口。它不实现 CiA402，也不等于 EtherCAT 主站协议栈。')
node('nic',60,690,240,92,'有线网卡 + RJ45',['Realtek 有线口','网线直连 EtherCAT IN'], 'cmd feedback config','实体通信出口。EtherCAT 直接使用二层以太网帧（EtherType 0x88A4），这条链路不依靠 TCP/UDP 或给从站分配 IP。2.5GbE 是网卡能力，不代表 EtherCAT 从站按 2.5 Gbit/s 工作。')
text(60,820,'Wireshark：观察帧与 WKC',17)
text(60,845,'网卡枚举 ≠ 已跑通通信',17,'#8051be',600)
wire('M180,390 V415','all',both=True);wire('M180,536 V562','all',both=True);wire('M180,658 V685','all',both=True)

node('phy',415,298,450,82,'RJ45 / 磁性器件 ↔ 集成 PHY',['网线电信号 ↔ 数字数据；IN / OUT 两个端口'], 'cmd feedback config','RJ45、网络磁性器件及配套电路属于模块硬件。LAN9252 内部集成双 Ethernet PHY，不应把它们误画为一定要额外购买的两颗芯片。OUT 可接下一从站；单从站实验无需把 OUT 用网线接回电脑。')
node('esc',415,410,450,93,'ESC 硬件处理核心',['报文 / 寻址 / WKC / 寄存器','数据流经 ESC；无需 STM32 转发整帧'], 'cmd feedback config clock','ESC 是 EtherCAT Slave Controller。硬件处理经过的帧、寻址与数据交换；从站应用层还需要 STM32 固件。MCU 内置普通 Ethernet MAC 不能直接代替 ESC。')
node('fmmu',415,532,450,81,'FMMU：地址映射硬件',['逻辑地址范围 → ESC 本地地址范围'], 'cmd feedback','只转换逻辑地址与本地数据区的对应关系。FMMU 不理解 0x607A，也不执行 PDO 解包。第九课用 EC_FMMU_Translate 模拟这个硬件功能。', '#edf3ff')
node('mailbox',415,642,213,105,'SM0 / SM1 邮箱区',['主站请求 / 从站响应','SDO 用逐笔握手'], 'config','常见配置：SM0 主站写、PDI 读；SM1 PDI 写、主站读。SyncManager 管理 DPRAM 的一段区域。SDO 请求通常按配置的本地邮箱地址访问，不必经过 PDO 的逻辑地址 FMMU。编号与地址由具体从站配置决定。','#f5efff')
node('process',650,642,215,105,'SM2 / SM3：PDO',['Rx：总线写，MCU 读','Tx：MCU 写，总线读'], 'cmd feedback','本例常见分配为 SM2 RxPDO 与 SM3 TxPDO。SM 是访问管理硬件，DPRAM 才是存储字节的 RAM。过程数据通常用硬件三缓冲，提供一致的最新值；第九课只用独立数组和顺序调用模拟，未实现真实 DPRAM 或三缓冲。','#eaf8f4')
node('pdi',415,780,450,61,'PDI：MCU 访问 ESC 的本地接口',[], 'cmd feedback config','PDI 是 Process Data Interface。本路线选择 SPI PDI，STM32 用驱动读写 ESC 寄存器和数据区；SPI 是本地接口，EtherCAT 是网线上运行的通信协议，两者不是同一种总线。')
wire('M640,380 V405','all',both=True);wire('M640,503 V527','cmd',both=True,paths='cmd feedback');wire('M760,613 V637','cmd',both=True,paths='cmd feedback');wire('M522,747 V775','config',both=True);wire('M760,747 V775','cmd',both=True,paths='cmd feedback')
text(415,633,'DPRAM：SM 管理的邮箱 / 过程数据存储区',15)
wire('M435,503 H402 V694 H410','config',dash=True)
wire('M300,733 H353 V338 H410','all','EtherCAT 网线',321,396,both=True)

node('spi',985,298,550,82,'SPI / DMA / GPIO / 中断：硬件外设',['SCK / MOSI / MISO / CS；IRQ 与 SYNC0 单独接线'], 'cmd feedback config clock','STM32 的 SPI 外设访问 ESC。DMA 可搬运字节，GPIO 提供 CS、复位等控制。IRQ 通知事件，SYNC0 提供 DC 同步节拍，它们不是 SPI 的数据线。H743 后续还需处理 DMA 缓冲的 Cache 一致性与内存可访问性。')
node('stack',985,410,550,93,'从站协议栈：运行在 STM32 的软件',['PDI 驱动 / EtherCAT 状态机 / PDO 编解码','Mailbox → CoE → SDO 服务'], 'cmd feedback config safety','固件承担启动检查、状态管理、PDO 编解码以及 CoE/SDO 服务。已有教学 C 模型帮助理解接口，但还不是可直接烧入板子的完整从站栈。真实迁移需硬件访问层、匹配 ESI/SII 和配置。')
node('od',985,532,550,111,'应用变量 ↔ 对象字典 OD',['target_position / actual_position 等共享数据','PDO 按布局读写；SDO 按索引、权限读写','OD 记录变量地址；并非另一份独立位置值'], 'cmd feedback config','第七课 EC_OD_Init(&od, &slave_command.target_position, &slave_feedback.actual_position) 绑定从站变量地址。PDO 解包更新目标，OD 和 SDO 随后访问同一变量。0x607A:00 是对象身份，不是 SPI 地址、ESC 地址或 C 指针。共享目标后续需要明确写入时机与并发规则。','#f5efff')
node('cia',985,672,550,80,'CiA402 + Joint Controller：应用软件',['控制字 / 状态字 / 模式 → 目标选择、限幅、使能'], 'cmd feedback safety','CiA402 定义驱动状态与命令语义；Joint Controller 把命令转为关节控制目标。EtherCAT OP 只是通信状态：还需要 CiA402 Operation Enabled 且保护条件通过，电机才可能使能。')
node('control',985,780,550,61,'电机控制：位置环 / 速度环 / 电流环 · FOC',[], 'cmd feedback safety','后续从模拟电机替换为控制算法。位置、速度、转矩模式在不同环路注入目标；电流环通常比 1 ms 通信循环快，并与 PWM / ADC 同步。算法运行在 STM32；能量转换由功率驱动硬件完成。')
wire('M1260,380 V405','all',both=True);wire('M1260,503 V527','all',both=True);wire('M1260,643 V667','cmd',both=True,paths='cmd feedback');wire('M1260,752 V775','cmd',both=True,paths='cmd feedback')
wire('M865,810 H922 V338 H980','all',both=True)
text(410,901,'SPI 是字节通道；IRQ 是事件通知；DC / SYNC0 是同步节拍。',21,'#172e45',600)
text(410,931,'EEPROM / SII：模块启动与身份配置；ESI XML：电脑侧设备说明。',19)
text(410,960,'OUT → 下一个关节从站 → …；链路末端由 ESC 内部回送帧。',19)

text(40,1001,'下半部分：运动与反馈',25,'#172e45',700)
for x,w,t in [(40,280,'④ 电源系统'),(390,500,'⑤ 驱动 → 电机 → 关节'),(960,600,'⑥ 传感器与本地反馈')]:
    rect(x,1025,w,305,'#eaf0f7','#c5d5e5',18);text(x+18,1060,t,24,'#172e45',700)
node('power',60,1083,240,98,'直流供电 + DC/DC',['功率母线 → 驱动','逻辑电源 → ESC / MCU'], 'power','功率母线给电机功率级供电；DC/DC 为 ESC、MCU 与传感器提供所需逻辑电源。电压、接地和隔离方案由实际板卡决定。网线在本方案负责通信，不能把它当作电机供电来源。')
text(60,1213,'数字逻辑与功率回路分开',17);text(60,1243,'共享参考地 / 隔离按原理图',17);text(60,1273,'电源电压以实际模块为准',17)
node('driver',415,1083,450,78,'栅极驱动器 + MOSFET 三相功率桥',['PWM / 使能 → 功率开关 → 电机 U / V / W'], 'cmd power safety','STM32 的 PWM 是控制信号，不能直接向电机绕组提供功率。栅极驱动器驱动 MOSFET，功率桥把直流母线转换为三相输出。本图以 PMSM/BLDC 三相驱动作为后续拓展方案，实际电机与驱动尚未选型。')
node('motor',415,1194,450,98,'电机 → 减速器 → 关节输出轴',['执行运动；输出力矩受传动关系影响','制动器 / 输出轴编码器：后续按需扩展'], 'cmd feedback power','电机产生旋转与转矩，经减速器驱动关节。编码器计数需要换算为角度；减速比、零位、方向与单位必须定义。电机侧位置与关节输出轴位置可能不同，转矩通常由电流估算，精确关节力矩可能需要额外传感器。')
node('encoder',985,1083,550,78,'编码器 / Hall → STM32 TIM 或 SPI 等',['转子角度 / 位置 / 速度；输出轴编码器可选'], 'feedback','反馈来自真实传感器。增量编码器可用定时器编码器模式，绝对编码器可能用 SPI 或其他接口，取决于所选器件。速度可由位置变化估算；FOC 需要合适的转子电角度信息。')
node('adc',985,1194,550,98,'电流 / 母线电压 / 温度 → ADC',['采样电路 + ADC / DMA → 控制与保护','转矩反馈可由模型与电流估算'], 'feedback safety','电流采样支撑电流闭环与过流判断；电压和温度支撑运行诊断与保护。ADC 采样常与 PWM 定时器同步。功率桥的电流与电机/关节位置来自不同测量链路，不能只靠 EtherCAT 推算。')
wire('M320,1130 H410','power','功率供电',326,1111)
wire('M300,1160 H343 V854 H410','power','逻辑供电',325,1018,dash=True)
wire('M343,1160 V880 H945 V854 H980','power',dash=True)
wire('M640,1161 V1189','power')
wire('M1170,841 V984 H845 V1078','cmd','PWM / EN：STM32 → 驱动',865,1010)
wire('M865,1240 H927 V1122 H980','feedback','机械测量',871,1191)
wire('M865,1122 H910 V1240 H980','feedback')
wire('M1535,1240 H1552 V808 H1539','feedback','传感器 → 控制环',1280,958)
add('<g class="wire" data-path="feedback"><path d="M1535,1122 H1552" fill="none" stroke="#0f8b79" stroke-width="3"/><circle cx="1552" cy="1122" r="4" fill="#0f8b79"/></g>')

rect(40,1378,1520,180,'#fff','#d6e1eb',18)
text(60,1412,'⑦ 横跨全系统：同步、保护与调试',24,'#172e45',700)
node('sync',60,1434,465,98,'DC / SYNC0 + FreeRTOS 调度',['同步“何时更新”；IRQ 提醒“有何事件”','1 ms 是通信目标，需测量周期与抖动'], 'clock','DC 单元在 ESC 中维护同步时钟，SYNC0 可接 STM32 外部中断触发周期更新。FreeRTOS 协调通信、控制、保护与监控任务；实时电流环通常由定时器/ADC中断承担。普通 Windows Sleep(1) 不证明严格 1 ms。','#fff8e9')
node('safe',550,1434,465,98,'通信 Watchdog + 电机保护',['PDO 超时 / 过流 / 过温 / 编码器异常','禁能或受控停机 → 故障状态与反馈'], 'safety','通信 watchdog 检测过程数据失联；MCU IWDG 检测程序卡死；本地保护处理过流、过压、过温、传感器异常等。软件禁能、硬件关断和制动必须按具体系统协调；一般 watchdog 不等同于认证 STO。','#fff0f1')
node('debug',1040,1434,500,98,'ST-LINK / UART / 示波器 / Wireshark',['SWD 看 MCU；逻辑分析仪看 SPI / IRQ','示波器看 PWM / ADC / SYNC0 时序'], 'all','ST-LINK 通过 SWD 下载和调试 MCU；UART 输出诊断日志；逻辑分析仪观察 SPI 字节与中断；示波器观察同步和功率控制时序；Wireshark 观察 EtherCAT 帧、地址、WKC。它们观察不同层，不能互相替代。')
text(40,1593,'蓝：命令   绿：反馈   紫：配置   金：同步   橙：供电   红：保护；彩色线表示关系，非 PCB 引脚接线图。',19)
text(40,1618,'范围：项目已规划内容 + 硬件迁移必需知识；当前仍是纯 C 模拟器，第九课学习中。2026-10-03',17)
add('</svg>')
svg = ''.join(parts)
(ROOT/'硬件关系总图.svg').write_text(svg,encoding='utf-8')

def flow(title,subtitle,items,color='cmd'):
    cards=''.join(f'<div class="flow-item"><b>{esc(a)}</b><span>{esc(b)}</span></div>'+('<span class="flow-arrow" aria-hidden="true">→</span>' if i<len(items)-1 else '') for i,(a,b) in enumerate(items))
    return f'<article class="flow-box {color}"><h3>{title}</h3><p>{subtitle}</p><div class="flow">{cards}</div></article>'
flows = flow('A. 周期命令：我要关节转到哪里？','从站视角：RxPDO = 从站接收。主站 outputs 是发出的命令；参数由主站打包，从站按相同布局解包。',[
('PC 目标值','Controlword + Target Position'),('主站打包 / 过程映像','outputs → EtherCAT 帧'),('ESC 总线侧','逻辑地址 → FMMU'),('SM2 / DPRAM','发布完整命令字节'),('SPI PDI → MCU','读取 → UnpackRxPDO'),('共享应用变量','target_position 更新'),('CiA402 / 关节控制','状态允许 → 控制目标'),('PWM / 功率级 / 电机','产生运动')])
flows += flow('B. 周期反馈：关节现在实际在哪里？','从站视角：TxPDO = 从站发送。主站 inputs 接收反馈。它走的是同一条双向网络链路，不需要另接一根“反馈网线”。',[
('编码器 / 电流采样','实际测量 / 状态估算'),('STM32 应用','actual_position / statusword'),('PackTxPDO','变量 → 固定布局字节'),('SPI PDI','把反馈写入 SM3 数据区'),('ESC 总线侧','FMMU 对应逻辑区 → 帧'),('PC 主站 inputs','解包 → 显示与诊断')],'feedback')
flows += flow('C. 参数配置：读对象、改模式、设置参数','SDO 是请求—响应服务。Upload = 主站读；Download = 主站写。可以在允许运行的通信状态下使用，通常不承担每周期运动指令。',[
('PC SDO 请求','Index / SubIndex / 数据'),('EtherCAT 邮箱','常见 SM0：主站写入'),('MCU 邮箱服务','经 PDI 取出请求'),('CoE → SDO 解析','解析服务、对象身份与字节'),('OD 查表与权限','找到共享变量 / 读写或拒绝'),('应答 / Abort','Mailbox SM1 → 主站')],'config')
flows += flow('D. 同步与保护：何时更新，何时停止？','下面是概念流程；真实时序由 DC 配置、应用设计与实测结果决定。',[
('ESC DC / SYNC0','提供周期同步事件'),('STM32 中断 / 通知','触发一次周期更新'),('通信与应用更新','取 Rx → CiA402 → 关节 → Tx'),('控制 ISR + ADC / PWM','更快的本地控制闭环'),('Watchdog / 保护','失联或故障 → 禁能 / 停机')],'clock')

rows = [
('PC Process Image','主站 RAM 中的字节缓冲','outputs / inputs','本机缓冲，与从站内存分开'),
('逻辑地址','EtherCAT 逻辑地址空间','0x00000000 / 0x00000010','FMMU 将其映射到 ESC 本地数据区'),
('ESC 本地地址','ESC 寄存器 / RAM 地址空间','0x1000 / 0x1010','本例教学选值，不是全部从站的固定地址'),
('PDO 字节偏移','字段在打包数据中的位置','偏移 2 开始的四字节目标位置','PDO 布局决定怎样解释字节'),
('对象索引 : 子索引','OD 中对象的身份','0x607A:00 / 0x6064:00','软件查表找到变量；不是内存地址'),
('C 指针','本机对象在 C 程序中的地址','&slave_command.target_position','仅在本进程/本 MCU 有意义，不经网线传递'),
('PDO Mapping / Assignment','对象字段与 PDO / SM 的配置关系','0x1600 / 0x1A00；0x1C12 / 0x1C13','常见 CoE 配置对象；需设备支持，当前布局固定')]
address_table=''.join('<tr>'+''.join(f'<td>{esc(v)}</td>' for v in row)+'</tr>' for row in rows)
objects=[('0x6040','Controlword','控制字','主站 → 从站','CiA402 驱动状态命令'),('0x6041','Statusword','状态字','从站 → 主站','CiA402 实际驱动状态'),('0x6060 / 0x6061','Mode / Mode Display','模式 / 模式显示','双向各一项','命令模式与当前模式'),('0x607A / 0x6064','Target / Actual Position','目标 / 实际位置','双向各一项','当前 OD 已支持这两个 INTEGER32 对象'),('0x60FF / 0x606C','Target / Actual Velocity','目标 / 实际速度','双向各一项','后续扩展，当前尚未实现'),('0x6071 / 0x6077','Target / Actual Torque','目标 / 实际转矩','双向各一项','后续扩展；单位、缩放与反馈估算需定义')]
object_table=''.join('<tr>'+''.join(f'<td>{esc(v)}</td>' for v in row)+'</tr>' for row in objects)

lessons=[
('01','整体架构','主站 → ESC → PDI → MCU → 应用','已学习'),('02','Frame / Datagram / WKC','网线上传什么，如何确认从站参与处理','已学习'),('03','寻址','自增 / 配置站地址 / 逻辑地址','已学习'),('04','ESC 内部结构','PHY、FMMU、SM、DPRAM 与 PDI','已学习'),('05','EtherCAT 状态机','INIT / PRE-OP / SAFE-OP / OP','已学习'),('06','PDO / Process Image','命令和反馈打包，小端与字段偏移','已完成'),('07','对象字典 OD','索引、子索引、类型、权限与共享变量指针','已完成'),('08','Mailbox / CoE / SDO','语义请求与四字节 expedited SDO 编解码','已完成'),('09','SyncManager / FMMU','逻辑地址 → 本地区域 → PDI → 应用变量','学习中'),('10','CiA402 基础','Controlword 0x6040 / Statusword 0x6041','后续规划'),('11','CiA402 状态机','驱动使能、Quick Stop、Fault 与状态反馈','后续规划'),('12','位置 / 速度 / 转矩模式','关节模型、目标选择与三种模式接口','后续规划'),('13','1 ms 周期控制','Rx → 状态机 → 控制 → Tx → 保护与诊断','后续规划'),('后续','FreeRTOS 架构','任务、中断、优先级、通知、共享数据一致性','阶段规划，课号未定'),('后续','STM32 + ESC 移植','SPI PDI、启动、ESI/SII、真实从站栈与主站验收','阶段规划，课号未定'),('拓展','真实电机控制','PWM、ADC、编码器、位置/速度/电流环与 FOC','按硬件分步开展'),('拓展','多关节 / DC / 工程诊断','链式从站、时钟同步、WKC、超时、抖动和测量','硬件迁移相关拓展'),('旁支','CAN / CANopen 对照','复用 OD、PDO/SDO、CiA402 思想；物理总线不同','已有基础与可选对照')]
lesson_table=''.join('<tr>'+''.join(f'<td>{esc(v)}</td>' for v in row)+'</tr>' for row in lessons)

steps = [
{'name':'① 电脑准备命令','path':'cmd','title':'目标值还只是电脑内存中的一个数字','body':'主站应用把 Controlword 与 Target Position 写进主站过程映像。第九课开始版本目标为 9000：命令六字节是 00 00 28 23 00 00。前两字节是控制字，后四字节是小端目标值。'},
{'name':'② ESC 接收字节','path':'cmd','title':'逻辑地址映射到 ESC 的本地数据区','body':'教学示例逻辑 0x00000000 → 本地 0x1000，共 6 字节，由 FMMU 换算。SM2 管理这段命令数据的交接。真实硬件上这些功能属于 ESC；当前模型用 C 代码与数组模拟。'},
{'name':'③ STM32 读取并解包','path':'cmd','title':'这一步才更新 target_position','body':'真实 STM32 经 SPI PDI 读出命令字节，再按 PDO 布局解包。第九课当前程序以 PDI 侧函数调用代替实际 SPI。SM 数据区里有 9000 的字节，不等于 C 变量已自动更新。'},
{'name':'④ 字典看到同一变量','path':'config','title':'OD 不是另拷贝一份目标位置','body':'EC_OD_Init 接收 &od 与两个位置变量的地址。OD 的条目保存这些地址；PDO 解包更新目标后，本地 OD 读取 0x607A:00 也看到 9000。目标对象的身份、ESC 本地地址与 C 指针是三回事。'},
{'name':'⑤ 状态允许才执行','path':'safety','title':'通信 OP 还不足以让电机转动','body':'后续 CiA402 解读控制字，更新驱动状态；只有通信状态、驱动使能与本地保护条件都满足，应用才允许执行。当前尚未实现 CiA402、运动模型、PWM 或真实电机。'},
{'name':'⑥ 采样并返回反馈','path':'feedback','title':'实际位置由反馈路径生产','body':'真实系统用编码器生成位置反馈。当前程序以固定 300 模拟反馈：TxPDO 六字节为 00 00 2C 01 00 00，经 SM3 / FMMU 返回主站。发出 9000 不会自动让反馈变成 9000。'},
{'name':'⑦ 另走 SDO 配置','path':'config','title':'读取对象或改参数走邮箱请求—响应','body':'主站 Upload 是读取，Download 是写入。请求沿 Mailbox → CoE → SDO → OD 查表；成功则返回数据或写确认，失败则返回 Abort。真实外层头、分段和超时等仍待后续完善。'}]

css='''
:root{--ink:#173047;--muted:#526579;--line:#d7e2ed;--blue:#2563eb;--bg:#f3f6fa}*{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0;background:var(--bg);color:var(--ink);font:16px/1.8 "Microsoft YaHei","PingFang SC",sans-serif}a{color:#2357aa}header{background:#142c43;color:#fff;padding:38px max(24px,calc((100vw - 1380px)/2));border-bottom:5px solid #67cfba}header .eyebrow{color:#90dfd0;letter-spacing:3px;font-size:13px}h1{font-size:clamp(27px,3.5vw,44px);line-height:1.3;margin:12px 0}header p{max-width:960px;color:#dce7f3;margin-bottom:12px}.badges{display:flex;flex-wrap:wrap;gap:10px}.badge{border:1px solid #526b82;border-radius:25px;padding:3px 12px;font-size:13px;color:#e9f1fa}nav{display:flex;gap:20px;flex-wrap:wrap;padding:12px 24px;background:#fff;border-bottom:1px solid var(--line);position:sticky;top:0;z-index:10}nav a{text-decoration:none;font-size:14px}main{max-width:1450px;padding:18px 24px 64px;margin:auto}section{margin-top:30px;scroll-margin-top:85px}h2{font-size:27px;margin:0 0 10px}h3{font-size:19px;margin:0 0 8px}.lead{color:var(--muted);margin:0 0 18px}.panel{background:#fff;border:1px solid var(--line);border-radius:16px;padding:24px}.toolbar{display:flex;gap:9px;align-items:center;flex-wrap:wrap;margin-bottom:15px}button,.button{font:inherit;font-size:14px;padding:7px 14px;border:1px solid #cbd9e5;border-radius:9px;background:white;color:#29465f;cursor:pointer;text-decoration:none}button:hover,.button:hover{background:#edf3fa}button.active{background:#163c63;color:white;border-color:#163c63}button:disabled{opacity:.4;cursor:default}.toolbar .spacer{flex:1}.diagram-wrap{overflow:auto;border:1px solid var(--line);border-radius:14px;background:#f4f7fb}.diagram-wrap svg{display:block;width:100%;min-width:1000px;height:auto}.detail{border-left:4px solid #2563eb;background:#eef4fc;padding:14px 20px;margin-top:16px;min-height:100px}.detail h3{margin-bottom:3px}.detail p{margin:0}.hint{font-size:13px;color:var(--muted);margin:10px 0}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:18px}.card{background:#fff;border:1px solid var(--line);border-radius:14px;padding:22px}.card p{margin:6px 0;color:var(--muted)}.flow-box{margin-top:18px;background:white;padding:22px;border:1px solid var(--line);border-left:5px solid #2563eb;border-radius:14px}.flow-box.feedback{border-left-color:#0f8b79}.flow-box.config{border-left-color:#8051be}.flow-box.clock{border-left-color:#b07808}.flow-box p{margin:0 0 15px;color:var(--muted)}.flow{display:flex;flex-wrap:wrap;gap:10px;align-items:center}.flow-item{background:#f2f5fa;padding:12px 14px;border-radius:9px;flex:1 0 155px}.flow-item b{display:block;font-size:15px}.flow-item span{display:block;font-size:13px;color:var(--muted)}.flow-arrow{color:#547698;font-size:22px}.table-wrap{overflow-x:auto;border-radius:12px;border:1px solid var(--line);background:white}table{border-collapse:collapse;width:100%;font-size:14px}th,td{text-align:left;padding:12px 15px;border-bottom:1px solid #e5ecf3;vertical-align:top}th{background:#eaf0f7;color:#203d56;white-space:nowrap}tr:last-child td{border-bottom:0}td:first-child{font-weight:600}code{background:#eaf0f6;color:#23455f;border-radius:4px;padding:2px 5px;font-family:Consolas,"Microsoft YaHei",monospace;font-size:.94em}pre{background:#152e43;color:#e2eefb;padding:18px;border-radius:12px;white-space:pre-wrap;overflow:auto;line-height:1.7}pre code{background:none;color:inherit;padding:0}.callout{background:#e8f5f1;border-left:4px solid #0f8b79;padding:15px 20px;border-radius:8px;margin:18px 0}.note{background:#fff7e8;border-left-color:#b07808}.states{display:flex;gap:8px;flex-wrap:wrap;align-items:center;margin:14px 0}.states span{background:#eff3f8;border-radius:8px;padding:5px 10px;font-size:13px}.steps{display:flex;flex-wrap:wrap;gap:7px;margin-bottom:15px}.step-content{background:#eef4fc;padding:20px;border-radius:12px;min-height:135px}.step-content p{margin:0}.step-controls{margin-top:12px;display:flex;gap:10px;align-items:center}.step-controls span{font-size:13px;color:var(--muted)}details{background:white;border:1px solid var(--line);padding:15px 20px;border-radius:12px;margin:10px 0}summary{cursor:pointer;font-weight:600}details p{margin:8px 0;color:var(--muted)}.source-list a{display:block;margin:5px 0}.foot{font-size:13px;color:var(--muted);margin-top:30px}ul,ol{padding-left:24px}li{margin:5px 0}.mini{font-size:13px;color:var(--muted)}@media(max-width:760px){main{padding:12px 14px 40px}.grid{grid-template-columns:1fr}.panel{padding:14px}nav{gap:10px;padding:8px 14px}h2{font-size:23px}}@media print{nav,.toolbar,.steps,.step-controls,.hint{display:none}body{background:white;font-size:11px}header{padding:16px;background:white;color:#173047}header p,.badge{color:#173047}main{max-width:none;padding:0}.diagram-wrap svg{min-width:0}section{break-before:page}section:first-child{break-before:auto}.panel,.card{box-shadow:none;padding:12px}.dim{opacity:1!important}.table-wrap{overflow:visible}details{break-inside:avoid}.flow-box{break-inside:avoid}a{color:inherit}}
'''

body='''
<header><div class="eyebrow">ETHERCAT → ROBOT JOINT · 学习导航图</div><h1>一张图看懂：硬件怎样连接，软件怎样让关节动起来</h1><p>把你的最终项目拆成“通信搬运字节、MCU 解释命令、控制算法计算、功率级执行、传感器反馈”。先定位每个模块，再沿一条数据路径理解它，不需要一次掌握整套协议栈。</p><div class="badges"><span class="badge">目标：STM32 + FreeRTOS + ESC 单关节控制器</span><span class="badge">第 08 课已完成 · 第 09 课学习中</span><span class="badge">2026-10-03 · 完全离线 · 无需安装软件</span></div></header>
<nav aria-label="内容导航"><a href="#overview">① 总图</a><a href="#walk">② 跟着数据走</a><a href="#paths">③ 四条通路</a><a href="#memory">④ 协议与内存</a><a href="#runtime">⑤ 状态与实时控制</a><a href="#hardware">⑥ 硬件与调试</a><a href="#roadmap">⑦ 学习路线</a><a href="#faq">⑧ 易混概念</a></nav><main>
<section id="overview"><h2>① 总图：先认位置，再认职责</h2><p class="lead">从上面的五个大模块看起。点击下面任意白色模块查看说明；选择颜色只突出相应通路。实体硬件内部的“软件框”表示程序运行的位置。</p><div class="panel"><div class="toolbar" id="filters"><button class="active" data-filter="all">全部关系</button><button data-filter="cmd">蓝 · 命令</button><button data-filter="feedback">绿 · 反馈</button><button data-filter="config">紫 · 配置 / SDO</button><button data-filter="clock">金 · 同步</button><button data-filter="power">橙 · 供电</button><button data-filter="safety">红 · 保护</button><span class="spacer"></span><button id="zoom">放大 / 适应</button><a class="button" href="硬件关系总图.svg" download>下载 SVG 总图</a><button id="print">打印 / 保存 PDF</button></div><div class="diagram-wrap" id="diagram">__SVG__</div><p class="hint">窄屏可以横向滚动。放大后看芯片内部；在打印对话框中可保存 PDF。图中线路是功能关系，具体引脚与供电按所选板卡原理图确定。</p><div class="detail" id="detail" aria-live="polite"><h3>先记住这一条主线</h3><p>PC 生成目标 → ESC 交换字节 → STM32 解包和控制 → 功率级驱动电机 → 编码器 / 采样反馈 → STM32 打包 → ESC → PC。供电与保护支撑整个系统。</p></div></div><div class="callout">当前代码没有真实 ESC、电机或网络传输。图中的硬件结构是最终目标架构；第九课数组代表数据区，函数调用代表两侧交接，不能把它们当作已经跑通的板卡。</div></section>
<section id="walk"><h2>② 用你现在的“目标 9000、实际 300”走一遍</h2><p class="lead">按顺序点击。数字取自第九课开始版本；以后练习改目标时，跟着你当前文件的值理解。</p><div class="panel"><div class="steps" id="steps"></div><div class="step-content" id="step-content" aria-live="polite"></div><div class="step-controls"><button id="prev">上一步</button><button id="next">下一步</button><span id="step-number"></span><a href="#overview">回总图看突出模块 ↑</a></div></div></section>
<section id="paths"><h2>③ 四条通路：数据、配置、时间各有职责</h2><p class="lead">它们共用同一套硬件，但任务不同。PDO 与 SDO 都能关联对象字典中的对象，传输形式与处理路径不同。</p>__FLOWS__<div class="callout note">PDO 周期数据通常走过程数据通道，不是每一笔都封装成“邮箱 → CoE 头 → SDO”。CoE 提供 CANopen 的对象字典与 PDO 配置思想；SDO 服务通过 CoE 邮箱访问对象。</div></section>
<section id="memory"><h2>④ 把协议、地址、字节和变量分清楚</h2><div class="grid"><article class="card"><h3>硬件传输层与软件服务层</h3><pre>网线上的 EtherCAT 帧
-- 周期过程数据 --> SM2 / SM3 --> PDO 编解码
-- 邮箱消息 --> SM0 / SM1 --> CoE --> SDO --> OD

PDO 编解码  &lt;--&gt;  共享应用变量  &lt;--&gt;  OD / SDO
                               |
                               +--> CiA402 / 关节应用</pre><p>SM0–SM3 是本图采用的常见分配，不是所有设备的强制编号。邮箱与过程数据区属于 ESC；CoE、SDO、OD 与 CiA402 属于从站软件。<a href="https://www.ethercat.org/en/technology.html">ETG：协议与设备描述</a></p></article><article class="card"><h3>ESC 内部：不能把四个名字当成四份数据</h3><pre>FMMU     地址翻译：哪一段逻辑地址对应哪一段本地地址
SM       访问管理：谁读、谁写、何时交付一致数据
DPRAM    数据存储：这里才保存收到 / 待发的字节
PDI      本地接口：MCU 怎样读写 ESC

SM 管理 DPRAM 的一段区域
MCU 通过 PDI 访问它，而不是用 PC 内存指针</pre><p>真实过程数据通常使用三缓冲保持一致快照；邮箱通常使用单缓冲握手。当前模型并没有真实硬件缓冲。<a href="https://infosys.beckhoff.com/content/1033/tc3_io_intro/4981170059.html">Beckhoff：FMMU / SM</a></p></article></div><h3 style="margin-top:22px">七种“数字”，分别属于哪里？</h3><div class="table-wrap"><table><thead><tr><th>概念</th><th>属于哪里</th><th>例子</th><th>怎样建立关系</th></tr></thead><tbody>__ADDRESS__</tbody></table></div><div class="callout"><b>第九课的换算：</b>本地地址 = 本地起点 +（请求逻辑地址 − 逻辑起点）。目标位置在命令 PDO 的偏移 2，所以逻辑 <code>0x00000002</code> 对应本地 <code>0x1002</code>，后续连续四字节。软件再根据布局解包到目标变量；它不是“对象 0x607A 直接映射为地址 0x1002”。</div><h3>对象字典怎样连接到 CiA402 与控制变量？</h3><div class="table-wrap"><table><thead><tr><th>对象索引</th><th>英文名称</th><th>含义</th><th>典型数据方向</th><th>本项目位置</th></tr></thead><tbody>__OBJECTS__</tbody></table></div><p class="mini">上表描述对象身份。它们是否进入 PDO、采用何种字段布局，由映射与应用配置决定；不是表里所有对象都已经写进当前 6 字节 PDO。</p></section>
<section id="runtime"><h2>⑤ 两套状态机、两种节拍、一份明确的保护条件</h2><div class="grid"><article class="card"><h3>EtherCAT 状态机：通信准备好了没有？</h3><div class="states"><span>INIT</span>→<span>PRE-OP</span>→<span>SAFE-OP</span>→<span>OP</span></div><p>INIT 初始化；PRE-OP 配置邮箱与参数；SAFE-OP 输入过程数据可用、输出保持安全；OP 允许有效过程输出。ESC 提供相关寄存器与事件，从站栈 / 应用执行转换检查，不是 ESC 自动实现全部应用逻辑。</p><p>WKC 检查报文预期参与处理情况；它不能证明电机实际到位。链路、映射、看门狗与应用条件都影响是否能进入 OP。<a href="https://infosys.beckhoff.com/content/1033/tc3_io_intro/1446518411.html">Beckhoff：通信状态与分层</a></p></article><article class="card"><h3>CiA402 状态机：驱动允许出力了吗？</h3><div class="states"><span>Switch On Disabled</span>→<span>Ready To Switch On</span>→<span>Switched On</span>→<span>Operation Enabled</span></div><p>由 Controlword 请求，Statusword 报告。还要学习 Not Ready、Quick Stop Active、Fault Reaction Active、Fault 等状态与转换。上面只是正常使能路径，不是完整状态机。</p><p>项目应用许可示意：<code>通信 OP &amp;&amp; 驱动 Operation Enabled &amp;&amp; 本地保护正常</code>。许可成立后，仍由控制算法和功率硬件完成运动。</p></article></div><div class="flow-box"> <h3>通信 1 ms 与电机电流环，不是同一个循环</h3><div class="flow"><div class="flow-item"><b>主站通信周期</b><span>发送目标 / 接收反馈；记录周期、抖动和丢失</span></div><span class="flow-arrow">→</span><div class="flow-item"><b>从站同步更新</b><span>SM 事件或 DC / SYNC0 驱动一次应用更新</span></div><span class="flow-arrow">→</span><div class="flow-item"><b>本地控制 ISR</b><span>PWM / ADC 定时触发，更快地控制电流</span></div></div><p class="mini">DC 负责时钟同步与事件时刻；SM 负责数据交接一致性，两者解决不同问题。IRQ 和 SYNC0 的用途也不同。Windows 功能调试不能替代严格实时验收。<a href="https://infosys.beckhoff.com/content/1033/epp1518-0002/3997042315.html">Beckhoff：DC / SYNC0</a></p></div><div class="grid" style="margin-top:18px"><article class="card"><h3>FreeRTOS 的建议职责划分</h3><ul><li><b>EtherCATTask：</b>PDI、邮箱与过程数据交接。</li><li><b>MotorControlTask：</b>模式和目标管理；高速电流环可由定时器 / ADC ISR 执行。</li><li><b>SafetyTask：</b>失联、本地故障与停机策略。</li><li><b>MonitorTask：</b>计数、日志与诊断，避免阻塞高优先级路径。</li></ul><p>ISR 通知任务；共享数据用快照、临界区或适当同步。不要因为用了 RTOS 就认为数据天然一致或系统已经满足实时性。</p></article><article class="card"><h3>闭环控制在 MCU 内部怎样接起来？</h3><pre>位置目标 --> 位置环 --> 速度目标
速度目标 ------------> 速度环 --> 转矩 / 电流目标
转矩目标 ----------------------> 电流环 / FOC
                                               |
                                       PWM --> 功率桥 --> 电机

编码器 --> 位置、速度、转子角度反馈
电流采样 --> 电流反馈 / 转矩估算</pre><p>这是典型串级思路。模式决定目标注入点；位置零位、单位、减速比、限幅与方向需要统一。FOC 的变换、调节与 PWM / ADC 同步属于后续拓展。<a href="https://www.st.com/content/st_com/en/ecosystems/stm32-motor-control-ecosystem.html">ST：电机控制与采样</a></p></article></div></section>
<section id="hardware"><h2>⑥ 接板时看这张表：哪根线承担什么？</h2><p class="lead">硬件迁移优先复用你已有的 H743 与 ST-LINK。下表是接口职责图；没有实际模块原理图之前，不指定 MCU 引脚号或假定电压。</p><div class="table-wrap"><table><thead><tr><th>连接</th><th>接口 / 信号</th><th>用途与要学的内容</th></tr></thead><tbody>
<tr><td>PC ↔ LAN9252 模块 IN</td><td>RJ45 / 网线</td><td>链路、二层 EtherCAT 帧、从站扫描、寻址、WKC；单从站 OUT 可空置</td></tr>
<tr><td>LAN9252 ↔ STM32H743</td><td>SPI：SCK / MOSI / MISO / CS</td><td>本地 PDI 字节读写、寄存器 / DPRAM 访问、超时与错误检查</td></tr>
<tr><td>ESC → STM32H743</td><td>IRQ；SYNC0；复位线按模块要求</td><td>IRQ 报事件；SYNC0 提供同步时刻；复位与启动顺序按手册</td></tr>
<tr><td>EEPROM ↔ ESC</td><td>模块内配置存储接口，典型 I²C</td><td>SII / 启动配置 / 身份信息；与 PC 侧 ESI XML 配套，区别于 MCU 固件</td></tr>
<tr><td>STM32 → 功率驱动</td><td>定时器 PWM / EN；故障反馈 / BREAK</td><td>控制输出、死区与保护、定时器硬件关断；这是电机驱动拓展阶段</td></tr>
<tr><td>编码器 / Hall → STM32</td><td>TIM 编码器输入 / SPI 等，按器件选择</td><td>采集位置、速度、零位与转子角度；必要时加关节输出轴编码器</td></tr>
<tr><td>电流 / 电压 / 温度 → STM32</td><td>采样电路 → ADC；DMA 可选</td><td>电流闭环、转矩估算与保护，采样和 PWM 同步</td></tr>
<tr><td>电源 → 功率级 / 数字板</td><td>功率母线 / DC/DC / 逻辑供电</td><td>电源预算、逻辑电平、参考地和隔离；遵循实际板卡设计</td></tr>
<tr><td>ST-LINK ↔ STM32；UART ↔ PC</td><td>SWD / UART</td><td>下载、断点与变量观察、诊断日志；UART 不承担 EtherCAT 周期通信</td></tr>
</tbody></table></div><div class="grid" style="margin-top:18px"><article class="card"><h3>设备描述、配置与固件：三样东西</h3><p><b>ESI XML：</b>电脑配置工具读取的设备说明：支持什么、PDO 如何布局、允许什么模式。</p><p><b>SII / EEPROM：</b>从站在线可读的身份和启动配置。SOEM 常见发现流程读取这些信息，并不都要求先导入 ESI。</p><p><b>STM32 固件：</b>真正运行协议栈与关节应用的程序；不能用 ESI 替代，也不能用当前模拟器直接替代。</p><p>这三者要一致。LAN9252 集成双 PHY，但完整模块还需 RJ45 配套电路、时钟、供电与配置存储。<a href="https://www.microchip.com/en-us/product/lan9252">Microchip：LAN9252</a> · <a href="https://www.ethercat.org/en/technology.html">ETG：ESI / SII</a></p></article><article class="card"><h3>遇到问题，按层找证据</h3><ol><li>电源、复位、时钟与链路灯：硬件能否启动？</li><li>SPI / IRQ：MCU 能否读到正确 ESC 寄存器？</li><li>主站扫描：从站数量、身份、状态是否正确？</li><li>SDO：对象身份、返回数据、Abort 是否符合预期？</li><li>PDO / OP / WKC：映射、方向和有效数据是否一致？</li><li>SYNC0 / ADC / PWM：实际周期、抖动、超时与控制响应。</li></ol><p>Wireshark 看总线帧；逻辑分析仪看 SPI；示波器看时序；ST-LINK 看 MCU 程序。先证明哪一层通了，再继续上一层。</p></article></div><div class="callout note">当前真实硬件验收只到主站库构建与网卡枚举。扫描从站、真实 SDO/PDO、进入 OP、电机运行与严格 1 ms 都还未验证。软件 watchdog 也不等同于认证 STO；硬件急停 / 制动按后续实际系统设计。</div></section>
<section id="roadmap"><h2>⑦ 你会学到的内容，分别落在总图哪里？</h2><p class="lead">第 01–13 课按交接规划列出；当前进度以 README 和学习记录为准。后续阶段与拓展没有擅自编课号，也没有标记为已经完成。</p><div class="table-wrap"><table><thead><tr><th>课程 / 阶段</th><th>内容</th><th>它解决的关系</th><th>当前状态</th></tr></thead><tbody>__LESSONS__</tbody></table></div><div class="flow-box"><h3>实践顺序：先让数字跑通，再让电机动起来</h3><div class="flow"><div class="flow-item"><b>纯 C 模拟器</b><span>理解数据 / 内存 / 状态 / 周期，逐课加功能</span></div><span class="flow-arrow">→</span><div class="flow-item"><b>H743 + ESC</b><span>扫描 / SDO / PDO / OP；先传 LED 或模拟位置</span></div><span class="flow-arrow">→</span><div class="flow-item"><b>电机闭环</b><span>编码器、采样、PWM、三环控制与保护</span></div><span class="flow-arrow">→</span><div class="flow-item"><b>工程验证</b><span>1 ms 目标、同步、抖动、丢包、故障恢复、多关节</span></div></div></div><p class="mini">顺序沿用“先看懂、改一点、运行、解释，再逐步仿写”的学习约定。真实电机控制、电源与传感器部分是最终项目需要的扩展知识，不表示已确定完整硬件方案。</p></section>
<section id="faq"><h2>⑧ 最容易混淆的关系，随时回来查</h2>
<details open><summary>STM32、ESC 和 PHY 有什么不同？</summary><p>PHY 做电信号与数字数据转换；ESC 做 EtherCAT 的硬件通信处理；STM32 执行从站软件与控制算法。LAN9252 把 ESC 与双 PHY 集成在同一芯片内。三个职责仍然不同。</p></details>
<details><summary>为什么 OD 没初始化，传给 EC_OD_Init 的 od 也不等于 0？</summary><p><code>EC_ObjectDictionary od = {0};</code> 是创建一个成员置零的结构体。调用 <code>EC_OD_Init(&od, ...)</code> 传入它的地址。函数参数 <code>EC_ObjectDictionary *od</code> 保存该地址；<code>od == 0</code> 检查有没有传入对象，而不是对象成员是否为零。对象字典还会保存位置变量的指针。</p></details>
<details><summary>PDO 和 OD 是否会保存两份 target_position？</summary><p>当前项目没有两份独立的从站目标位置：PDO 解包更新 <code>slave_command.target_position</code>，OD 保存它的地址，SDO 经 OD 访问同一个变量。传输字节缓冲、主站目标变量和从站应用变量是不同存储；“共享”仅指从站 PDO/OD 访问同一应用数据。</p></details>
<details><summary>EtherCAT OP 和 CiA402 Operation Enabled 是一个状态吗？</summary><p>不是。OP 属于通信状态；Operation Enabled 属于驱动状态。两个状态加上本地保护条件共同约束应用是否允许执行。不能看到 OP 就自动给功率级使能。</p></details>
<details><summary>CoE 为什么有 CANopen 这个词，要接 CAN 线吗？</summary><p>CoE = CANopen over EtherCAT：沿用 CANopen 的对象字典、PDO 配置与 SDO 服务机制，承载网络是 EtherCAT。此主线不用 CAN 收发器。若以后选用 CAN/CANopen 电机驱动，需另加 CAN 控制器 / 收发器和独立总线，作为另一种架构分支。</p></details>
<details><summary>FMMU、PDO Mapping 和 C 指针都是“映射”，是不是同一件事？</summary><p>不是。FMMU 关联逻辑地址与 ESC 本地地址；PDO Mapping 关联对象字段与字节 / 位布局；C 指针关联软件条目与本机变量地址。它们一层层把字节连接到应用值，但没有直接的数值相等关系。</p></details>
<details><summary>SYNC0、IRQ、SM 和 Watchdog 各解决什么？</summary><p>SYNC0 提供同步时刻；IRQ 通知待处理事件；SM 管理一致的数据交接；通信 Watchdog 检测过程数据超时。MCU IWDG 则处理程序卡死。它们不是一个模块的不同叫法。</p></details>
<details><summary>一根网线能同时下发命令和上传反馈吗？</summary><p>能。过程数据在同一 EtherCAT 链路双向交换。从站之间可从 OUT 串到下一站的 IN，末端内部回送帧。示意图上两条方向箭头表示数据关系，不要求给命令和反馈分别接网线。</p></details>
<details><summary>FOC、电流环和 CiA402 是一回事吗？</summary><p>CiA402 约定驱动状态与目标模式；控制环计算如何达到目标；FOC 处理电机电流和转矩相关控制；PWM 控制功率开关。它们位于不同层，只有联合起来才能产生可控运动。</p></details>
<details><summary>本图有没有覆盖所有 EtherCAT 标准？</summary><p>覆盖本项目已规划的主线与硬件迁移必需关系。FoE 固件传输、EoE 隧道、SoE、FSoE 等属于可选协议专题，当前课程未承诺全部实现。完整 Mailbox/CoE 外层、SDO 分段和超时重试仍需后续分步补足。图中“全部”指当前项目学习路线，不是所有工业协议。</p></details>
</section><section id="sources"><h2>本图依据</h2><p class="lead">进度来自项目当前记录，硬件与协议职责对照官方资料。以下链接需联网；本图的阅读与交互完全离线。</p><div class="grid"><article class="card source-list"><h3>本地课程来源</h3><a href="../../README.md">项目 README：当前第九课学习中</a><a href="../学习记录.md">学习记录：逐课成果与验证范围</a><a href="../../项目交接文档.md">交接文档：最终项目与第 10–13 课规划</a><a href="../../EtherCAT_Slave_Simulator/Lesson_09_SyncManager与FMMU.md">第九课：地址与 PDI 数据路径</a><a href="../../EtherCAT_Master_Lab/README.md">主站实验：硬件路线与实际验收状态</a></article><article class="card source-list"><h3>官方技术依据</h3><a href="https://www.microchip.com/en-us/product/lan9252">Microchip：LAN9252 集成双 PHY</a><a href="https://ww1.microchip.com/downloads/aemDocuments/documents/UNG/ProductDocuments/DataSheets/LAN9252-Data-Sheet-DS00001909.pdf">Microchip：LAN9252 数据手册 / PDI</a><a href="https://infosys.beckhoff.com/content/1033/tc3_io_intro/4981170059.html">Beckhoff：FMMU / SyncManager / 缓冲</a><a href="https://infosys.beckhoff.com/content/1033/tc3_io_intro/1446518411.html">Beckhoff：EtherCAT 分层与状态</a><a href="https://infosys.beckhoff.com/content/1033/epp1518-0002/3997042315.html">Beckhoff：DC / SYNC0 同步</a><a href="https://www.ethercat.org/en/technology.html">ETG：CoE、ESI 与 SII</a><a href="https://download.beckhoff.com/download/document/io/ethercat-development-products/an_et9300_v1i10.pdf">Beckhoff：从站应用与 CiA402 示例</a><a href="https://www.st.com/content/st_com/en/ecosystems/stm32-motor-control-ecosystem.html">ST：FOC、位置 / 速度 / 电流与传感器</a></article></div><p class="foot">生成日期：2026-10-03（中国时间）。这是学习辅助资料，未改变课程源码、练习数值、完课记录或课程标签。</p></section></main>
'''
body=body.replace('__SVG__',svg).replace('__FLOWS__',flows).replace('__ADDRESS__',address_table).replace('__OBJECTS__',object_table).replace('__LESSONS__',lesson_table)
js='''
const nodes=__NODES__;
const steps=__STEPS__;
const filters=document.querySelectorAll('[data-filter]');
function selectPath(path){
  filters.forEach(b=>{b.classList.toggle('active',b.dataset.filter===path);b.setAttribute('aria-pressed',String(b.dataset.filter===path));});
  document.querySelectorAll('#diagram [data-path]').forEach(el=>{const ps=el.dataset.path.split(' ');el.classList.toggle('dim',path!=='all'&&!ps.includes(path));});
}
filters.forEach(b=>b.addEventListener('click',()=>selectPath(b.dataset.filter)));
function showNode(el){
  document.querySelectorAll('#diagram .selected').forEach(n=>n.classList.remove('selected'));el.classList.add('selected');
  const n=nodes[el.dataset.id]; const d=document.getElementById('detail');d.replaceChildren();
  const h=document.createElement('h3');h.textContent=n.title;const p=document.createElement('p');p.textContent=n.detail;d.append(h,p);
}
document.querySelectorAll('#diagram .node').forEach(el=>{el.addEventListener('click',()=>showNode(el));el.addEventListener('keydown',e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();showNode(el);}});});
let zoomed=false;document.getElementById('zoom').addEventListener('click',()=>{zoomed=!zoomed;document.querySelector('#diagram svg').style.width=zoomed?'1600px':'100%';document.getElementById('zoom').setAttribute('aria-pressed',String(zoomed));});
document.getElementById('print').addEventListener('click',()=>{selectPath('all');window.print();});
let index=0;
steps.forEach((s,i)=>{const b=document.createElement('button');b.textContent=s.name;b.dataset.step=String(i);b.addEventListener('click',()=>showStep(i));document.getElementById('steps').append(b);});
function showStep(i){index=Math.max(0,Math.min(steps.length-1,i));const s=steps[index];document.querySelectorAll('[data-step]').forEach(b=>{b.classList.toggle('active',Number(b.dataset.step)===index);b.setAttribute('aria-pressed',String(Number(b.dataset.step)===index));});const d=document.getElementById('step-content');d.replaceChildren();const h=document.createElement('h3');h.textContent=s.title;const p=document.createElement('p');p.textContent=s.body;d.append(h,p);document.getElementById('prev').disabled=index===0;document.getElementById('next').disabled=index===steps.length-1;document.getElementById('step-number').textContent=`第 ${index+1} / ${steps.length} 步`;selectPath(s.path);}
document.getElementById('prev').addEventListener('click',()=>showStep(index-1));document.getElementById('next').addEventListener('click',()=>showStep(index+1));
showStep(0);selectPath('all');
'''.replace('__NODES__',json.dumps(nodes,ensure_ascii=False)).replace('__STEPS__',json.dumps(steps,ensure_ascii=False))
out=f'<!DOCTYPE html><html lang="zh-CN"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>EtherCAT 单关节：硬件与知识关系图</title><style>{css}</style></head><body>{body}<script>{js}</script></body></html>'
(ROOT/'硬件与知识关系图.html').write_text(out,encoding='utf-8')
(ROOT/'阅读指南.md').write_text('''# EtherCAT 硬件与知识关系图

生成于 2026-10-03，依据项目 README、学习记录、交接规划及图中列出的官方资料。

先双击 **硬件与知识关系图.html**，用浏览器离线阅读。

1. 先看总图顶端五个模块：PC → ESC → STM32 → 功率级 → 电机与关节。
2. 点击白色模块，查看职责与当前实现范围；选择命令、反馈、配置、同步、供电或保护，突出相应关系。
3. 跟着“目标 9000、实际 300”的七步演示理解数据流。
4. 再看 PDO / SDO 通路、地址对照、两套状态机与学习路线。
5. 回到第九课时，重点看 FMMU → SM 管理的数据区 → PDI → 解包 → 共享变量。

**硬件关系总图.svg** 是可无限放大的矢量总图；可用浏览器打开，或从 HTML 下载。
**硬件关系总图.png** 为浏览器导出的总图图片（若存在）。

图中的“全部”覆盖项目已规划主线及硬件迁移相关知识；没有把全部 EtherCAT 可选协议纳入必修。后续阶段与拓展没有擅自安排课号。当前第九课学习中，真实从站通信、电机闭环与严格 1 ms 尚未验证。

具体引脚、电源电压与驱动选型由实际硬件资料决定。图是关系框图，不是可照抄的 PCB 原理图。未修改源码、练习值与课程完成状态。

重新生成 HTML 和 SVG（Python 标准库）：

```powershell
python .\\docs\\硬件关系图\\生成图.py
```
''',encoding='utf-8')
print(f'已生成 HTML、SVG 与阅读指南；共 {len(nodes)} 个可点击模块、{len(steps)} 步演示、{len(lessons)} 项课程 / 阶段。')
