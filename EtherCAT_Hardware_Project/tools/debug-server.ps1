. (Join-Path $PSScriptRoot 'environment.ps1')
Write-Host 'OpenOCD 前台运行：另开 PowerShell 执行 tools/debug.ps1；结束用 Ctrl+C。'
Invoke-Checked $ToolPaths.openocd @('-f','interface/stlink.cfg','-f','target/stm32f4x.cfg','-c','adapter speed 1000')
