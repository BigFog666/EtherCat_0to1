# EtherCAT 从零到机器人关节控制器

学习目标：从纯 C 软件模拟器出发，逐步理解 EtherCAT、PDO/SDO、CoE 对象字典和 CiA402，后续迁移到 STM32 + FreeRTOS + ESC 的单关节控制器。

当前主线：**第八课第二小节：SDO 字节格式，学习中**。第一小节请求/响应语义模拟已学习；当前增加 4 字节对象的快速 SDO 内容编解码。第六、七课已完成。仓库中的 Windows SOEM 主站实验是辅助环境准备，尚未验证真实从站通信。

## 学习进度与课程版本

| 课程 | 内容 | 状态 | 固定版本 |
|---|---|---|---|
| 01–05 | 整体架构、报文、寻址、ESC、EtherCAT 状态机 | 已学习，见交接文档 | 没有当时的独立代码版本 |
| 06 | PDO 与 Process Image | 已完成 | [lesson-06](https://github.com/BigFog666/EtherCat_0to1/tree/lesson-06) |
| 07 | 对象字典 | 已完成 | [lesson-07](https://github.com/BigFog666/EtherCat_0to1/tree/lesson-07)；[开始版本](https://github.com/BigFog666/EtherCat_0to1/tree/lesson-07-start) |
| 08 | Mailbox、CoE 与 SDO | 第二小节学习中 | [第二小节起点](https://github.com/BigFog666/EtherCat_0to1/tree/lesson-08-part2-start)；[第一小节归档](https://github.com/BigFog666/EtherCat_0to1/tree/lesson-08-part1) |

第六课版本是在建立 Git 仓库时，根据本次聊天及现有文件重建的归档，不是当时已经存在的 Git 提交。保留了学习者完成的目标位置 2000、实际位置 300，以及中文注释和兼容的构建脚本。第七课开始版本不表示第七课已经完成。

## 目录导航

- [项目交接文档](项目交接文档.md)：第一至第五课、最终目标及课程规划。
- [纯 C 从站模拟器](EtherCAT_Slave_Simulator/README.md)：第六课说明及累计代码。
- [第七课对象字典说明](EtherCAT_Slave_Simulator/Lesson_07_对象字典.md)：已完课，保留练习成果。
- [第八课 Mailbox 与 SDO 说明](EtherCAT_Slave_Simulator/Lesson_08_Mailbox与SDO.md)：已学习的第一小节。
- [第八课第二步：SDO 字节格式](EtherCAT_Slave_Simulator/Lesson_08_第二步_SDO字节格式.md)：当前小节，包含布局表、流程图和新练习。
- [程序运行流程图](docs/程序运行流程图.md)：main 的整体顺序及一次 SDO 事务的展开图。
- [学习记录](docs/学习记录.md)：每课新增内容、练习、验证及版本来源。
- [Windows SOEM 主站实验](EtherCAT_Master_Lab/README.md)：网卡枚举和后续真实从站实验准备。

## 构建当前课程

在项目根目录的 PowerShell 中执行：

```powershell
.\EtherCAT_Slave_Simulator\build.ps1
```

需要 GCC 在 PATH 中。脚本会重新编译并运行；生成文件保存在 build 中，不上传 GitHub。当前运行包括第六课数据交换、第七课字典读写、第八课邮箱请求/响应语义模拟及快速 SDO 内容的字节编解码。

主站实验依赖 SOEM 和 Npcap。vendor 源码及本地构建目录不提交；重新准备方法、固定的 SOEM 提交和许可证位置见主站实验 README。

## 怎样回顾之前的课程

在 GitHub 上打开上表中的固定版本，可以直接阅读或下载该课代码，无需改变当前学习目录。

若想在本地运行第六课，建议创建独立工作目录，保留当前代码和未提交练习：

```powershell
git worktree add --detach ..\EtherCat_lesson06 lesson-06
```

然后进入新目录运行它的 build.ps1。工作目录位于当前项目的旁边，由学习者根据自己的保存位置选择。本仓库中的标签固定对应课程版本；后续不会用新代码覆盖已有课程标签。

## 后续归档方式

学习过程中可以提交阶段进度；学习者明确表示某节课完成后，检查代码和运行结果、更新学习记录、创建完课提交及 lesson-XX 标签，再推送 main 和该课标签到 GitHub。课程中遇到的问题和修正一起记入学习记录。

当前代码仍是教学模拟器，不代表已实现真实 EtherCAT 协议栈、硬件通信或严格 1 ms 实时控制。
