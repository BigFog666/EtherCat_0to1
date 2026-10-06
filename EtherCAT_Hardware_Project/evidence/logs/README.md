# 原始日志与备份

保存实际硬件 CSV、终端输出、STM32 Flash 和 EEPROM/SII 备份。此目录原始文件本地归档，不提交 Git；需要分享时挑选去除机器信息的摘要写到 evidence/measurements。

不要把 offline_demo 的输出命名为 hardware.csv。每次记录命令、工具/固件版本、板卡身份、接线、请求周期、是否暂停调试，以及成功或失败。

2026-10-05 首次真实联调已完成，摘要见 [验收报告](../2026-10-05_首次硬件联调.md)。`factory_stm32_20261005_152134.bin` 和 `factory_sii_20261005_first.bin` 为烧录前原始备份，请保留，后续读取另取文件名。三轮成功日志前缀为 `20261005_demo_refresh_state`、`20261005_demo_session_reset_01`、`20261005_demo_session_reset_02`；失败诊断日志也保留，不作为成功数据。
