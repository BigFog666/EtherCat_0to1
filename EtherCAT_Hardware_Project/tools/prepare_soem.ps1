param([string]$Source='')
. (Join-Path $PSScriptRoot 'environment.ps1')
if (-not $Source) { $Source = Join-Path (Split-Path -Parent $ProjectRoot) 'EtherCAT_Master_Lab/vendor/SOEM' }
$Source = (Resolve-Path -LiteralPath $Source).Path
$expected = '88e8ed46efba7dfa7b94d08a512db25a33e3f8d5'
$actual = & git -c "safe.directory=$($Source.Replace('\','/'))" -C $Source rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $actual -ne $expected) { throw 'SOEM 源码版本不符，拒绝复制。' }
$dest = Join-Path $ProjectRoot 'vendor/SOEM'
if (Test-Path -LiteralPath $dest) { throw 'vendor/SOEM 已存在，不覆盖；修改前保留本地成果。' }
New-Item -ItemType Directory -Path $dest | Out-Null
Get-ChildItem -LiteralPath $Source -Force | Where-Object { $_.Name -ne '.git' } | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $dest -Recurse
}
[IO.File]::WriteAllText((Join-Path $dest 'PROJECT_SOURCE_COMMIT.txt'), $expected, [Text.Encoding]::UTF8)
Write-Host "已复制固定 SOEM 依赖（不含 .git）：$dest"
