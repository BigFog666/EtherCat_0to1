param([switch]$BackupOnly)
. (Join-Path $PSScriptRoot 'environment.ps1')
Push-Location $ProjectRoot
try {
    $base = @('-f','interface/stlink.cfg','-f','target/stm32f4x.cfg','-c','adapter speed 1000')
    if ($BackupOnly) {
        $file = 'evidence/logs/factory_stm32_' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '.bin'
        Invoke-Checked $ToolPaths.openocd ($base + @('-c',"init; reset halt; dump_image $file 0x08000000 0x80000; shutdown"))
        Write-Host "已读取 STM32 512 KiB Flash：$file。开发板保持暂停，复位后恢复运行。"
    } else {
        $file = 'build/firmware-gcc/ethercat_joint.elf'
        if (-not (Test-Path -LiteralPath $file)) { throw '请先运行 tools/build.ps1。' }
        Invoke-Checked $ToolPaths.openocd ($base + @('-c',"program $file verify reset exit"))
    }
} finally { Pop-Location }
