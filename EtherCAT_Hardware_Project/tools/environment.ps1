$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ToolPaths = Get-Content -LiteralPath (Join-Path $ProjectRoot 'config/tool_paths.json') -Raw -Encoding UTF8 | ConvertFrom-Json
function Invoke-Checked {
    param([string]$Executable, [string[]]$Arguments)
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "命令失败（退出码 $LASTEXITCODE）：$Executable" }
}
function Use-Npcap {
    if (-not (Test-Path -LiteralPath (Join-Path $ToolPaths.npcap 'wpcap.dll'))) { throw '未找到 Npcap，请先核对 config/tool_paths.json。' }
    $env:PATH = $ToolPaths.npcap + ';' + $env:PATH
}
