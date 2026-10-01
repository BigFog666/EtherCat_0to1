# EtherCAT 从零到机器人关节控制器

这是第六课 PDO 与 Process Image 的完课归档，标签为 lesson-06。

本版本在 2026-10-01 建立 Git 仓库时，根据交接文档、当前聊天及现有文件重建，不是当时已存在的 Git 历史。保留目标位置 2000、模拟实际位置 300、中文注释和兼容的构建脚本；不包含第七课对象字典代码。

已学第一至第五课的记录见[交接文档](项目交接文档.md)。第六课的讲解和练习答案见[模拟器说明](EtherCAT_Slave_Simulator/README.md)，完课成果见[学习记录](docs/学习记录.md)。

在项目根目录运行：

```powershell
.\EtherCAT_Slave_Simulator\build.ps1
```

需要 GCC 在 PATH 中，脚本会编译并运行。该程序仅模拟状态切换和 PDO 数据交换，尚未实现真实网络、ESC/PDI、CiA402 或电机运动。

下一课：对象字典。后续每课保留提交和固定标签，可在 GitHub 选择相应标签查看当时代码。