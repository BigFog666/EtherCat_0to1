param(
    [ValidateSet('Info','Backup','Readback','WriteProject')][string]$Action='Info',
    [Parameter(Mandatory=$true)][string]$Interface,
    [string]$BackupPath='', [switch]$ConfirmProjectWrite
)
. (Join-Path $PSScriptRoot 'environment.ps1')
$OldPath = $env:PATH
Push-Location $ProjectRoot
try {
    Use-Npcap
    $exe = Join-Path $ProjectRoot 'build/host/eepromtool.exe'
    switch ($Action) {
        'Info' { Invoke-Checked $exe @($Interface,'1','-i') }
        'Backup' {
            if (-not $BackupPath) { $BackupPath = 'evidence/logs/factory_sii_' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '.bin' }
            if (Test-Path -LiteralPath $BackupPath) { throw '备份路径已存在，请换一个名称，避免覆盖。' }
            Invoke-Checked $exe @($Interface,'1','-r',$BackupPath)
            if (-not (Test-Path -LiteralPath $BackupPath)) { throw '未生成备份；不要继续写入。' }
            Write-Host "备份：$BackupPath。请保留 Info 输出并核对文件长度及 CRC；官方示例退出码不能证明读写成功。"
        }
        'Readback' {
            $file = 'evidence/logs/project_sii_readback_' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '.bin'
            Invoke-Checked $exe @($Interface,'1','-r',$file)
            Invoke-Checked $ToolPaths.python @('tools/verify_sii.py',$file,'--project')
        }
        'WriteProject' {
            if (-not $ConfirmProjectWrite) { throw '这是实际 EEPROM 写入。完成单板接线、原配置备份与 SPI 配置核对后，显式加 -ConfirmProjectWrite。' }
            if (-not $BackupPath -or -not (Test-Path -LiteralPath $BackupPath)) { throw '请用 -BackupPath 指定本块板的原 EEPROM 备份。' }
            Invoke-Checked $ToolPaths.python @('tools/verify_sii.py',$BackupPath)
            Invoke-Checked $ToolPaths.python @('tools/generate_sii.py')
            Invoke-Checked $exe @($Interface,'1','-w','build/config/EtherCAT_Joint_F407.sii.bin')
            $readback = 'evidence/logs/written_sii_' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '.bin'
            Invoke-Checked $exe @($Interface,'1','-r',$readback)
            Invoke-Checked $ToolPaths.python @('tools/verify_sii.py',$readback,'--project')
            Write-Host '写后读回相同；请断开板电源后重新上电，再扫描核对身份。'
        }
    }
} finally { $env:PATH = $OldPath; Pop-Location }
