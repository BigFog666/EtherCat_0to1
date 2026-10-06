param(
    [ValidateSet('Help','List','Scan','Inspect','Reset','Demo','LedDemo','Offline')][string]$Action='Help',
    [string]$Interface='', [int]$Cycles=2000, [int]$PeriodUs=10000,
    [string]$Csv='evidence/logs/hardware.csv', [switch]$ResetBeforeDemo
)
. (Join-Path $PSScriptRoot 'environment.ps1')
$OldPath = $env:PATH
Push-Location $ProjectRoot
try {
    if ($Action -eq 'Offline') { Invoke-Checked (Join-Path $ProjectRoot 'build/host/offline_demo.exe') @(); return }
    Use-Npcap
    $arguments = @('--' + $Action.ToLowerInvariant())
    if ($Action -eq 'LedDemo') { $arguments = @('--demo'); if (-not $PSBoundParameters.ContainsKey('Cycles')) { $Cycles=6000 } }
    if ($Action -notin @('Help','List')) {
        if ([string]::IsNullOrWhiteSpace($Interface)) { throw '请使用 -Interface 传入 List 输出中的有线网卡名称。' }
        $arguments += $Interface
    }
    if ($Action -eq 'LedDemo') { $arguments += '--visual' }
    if ($Action -in @('Demo','LedDemo')) { $arguments += @('--cycles',"$Cycles",'--period-us',"$PeriodUs",'--csv',$Csv) }
    if ($Action -in @('Demo','LedDemo') -and $ResetBeforeDemo) { $arguments += '--reset-start' }
    Invoke-Checked (Join-Path $ProjectRoot 'build/host/joint_master.exe') $arguments
} finally { $env:PATH = $OldPath; Pop-Location }
