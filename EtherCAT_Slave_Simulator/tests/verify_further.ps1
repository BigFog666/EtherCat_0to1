$ErrorActionPreference = 'Stop'
# 教师验证：全部产物和练习副本留在被 Git 忽略的 build 中，原课程代码不改。
$simulatorDirectory = Split-Path -Parent $PSScriptRoot
$outputDirectory = Join-Path $simulatorDirectory 'build'
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$modelDirectory = Join-Path $simulatorDirectory 'Further_Lessons'
$commonSources = @(
    (Join-Path $modelDirectory 'joint.c')
    (Join-Path $modelDirectory 'mode_pdo.c')
    (Join-Path $modelDirectory 'watchdog.c')
    (Join-Path $simulatorDirectory 'EtherCAT/ethercat_pdo.c')
    (Join-Path $simulatorDirectory 'EtherCAT/ethercat_state.c')
    (Join-Path $simulatorDirectory 'CiA402/cia402.c')
    (Join-Path $simulatorDirectory 'CiA402/cia402_state.c')
    (Join-Path $simulatorDirectory 'CiA402/cia402_fault.c')
)
function Invoke-VerifiedProgram {
    param([string]$Source, [string]$Name)
    $executable = Join-Path $outputDirectory ($Name + '.exe')
    $arguments = @('-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-I', $modelDirectory, $Source)
    $arguments += $commonSources
    $arguments += @('-o', $executable)
    & gcc @arguments
    if ($LASTEXITCODE -ne 0) { throw ('Compilation failed: ' + $Name) }
    $lines = @(& $executable)
    if ($LASTEXITCODE -ne 0) { throw ('Program failed: ' + $Name) }
    return $lines
}
function Assert-Output {
    param([string[]]$Lines, [string]$Pattern)
    if (-not ($Lines -match $Pattern)) { throw ('Expected output missing: ' + $Pattern) }
}
function Write-Variant {
    param([string]$Text, [string]$Old, [string]$New, [string]$Name)
    # 一次只替换唯一指定位置，防止误改多个初始化场景。
    if ([regex]::Matches($Text, [regex]::Escape($Old)).Count -ne 1) {
        throw ('Variant replacement must match exactly once: ' + $Name)
    }
    $path = Join-Path $outputDirectory ($Name + '.c')
    [System.IO.File]::WriteAllText($path, $Text.Replace($Old, $New), [System.Text.UTF8Encoding]::new($false))
    return $path
}

Invoke-VerifiedProgram (Join-Path $PSScriptRoot 'verify_further.c') 'verify_further'
Invoke-VerifiedProgram (Join-Path $PSScriptRoot 'verify_watchdog.c') 'verify_watchdog'
$source12 = Join-Path $modelDirectory 'lesson12.c'
$source13 = Join-Path $modelDirectory 'lesson13.c'
$before12 = (Get-FileHash -LiteralPath $source12 -Algorithm SHA256).Hash
$before13 = (Get-FileHash -LiteralPath $source13 -Algorithm SHA256).Hash
$text12 = [System.IO.File]::ReadAllText($source12)
$text13 = [System.IO.File]::ReadAllText($source13)

$baseline = Invoke-VerifiedProgram $source13 'verify_lesson13_baseline'
Assert-Output $baseline '^t=6, valid=0, expired=1, latched=1, EC=SAFE-OP, drive=SWITCH_ON_DISABLED, move=0, position=700$'
Assert-Output $baseline '^t=7, valid=1, expired=0, latched=1, EC=SAFE-OP, drive=SWITCH_ON_DISABLED, move=0, position=700$'
Assert-Output $baseline '^Diagnostics: accepted=9, missing=3, rejected=1, watchdog_trips=1, published=13$'

$variant12 = Write-Variant $text12 'int32_t lesson12_target_position = 1000;' 'int32_t lesson12_target_position = 500;' 'lesson12_target500'
$exercise12 = Invoke-VerifiedProgram $variant12 'lesson12_target500'
Assert-Output $exercise12 '^CSP tick=2, mode=8, position=500, velocity=100000, torque=0, sw=0x0027$'
Assert-Output $exercise12 '^CSP tick=10, mode=8, position=500, velocity=0, torque=0, sw=0x0027$'
Assert-Output $exercise12 '^CSV tick=10, mode=9, position=305, velocity=500, torque=0, sw=0x0027$'
Assert-Output $exercise12 '^CST tick=10, mode=10, position=305, velocity=1000, torque=100, sw=0x0027$'

$variant13 = Write-Variant $text13 'uint32_t lesson13_timeout_ms = 3;' 'uint32_t lesson13_timeout_ms = 5;' 'lesson13_timeout5'
$exercise13 = Invoke-VerifiedProgram $variant13 'lesson13_timeout5'
Assert-Output $exercise13 '^t=6, valid=0, expired=0, latched=0, EC=OP, drive=OPERATION_ENABLED, move=1, position=800$'
Assert-Output $exercise13 '^t=12, valid=1, expired=0, latched=0, EC=OP, drive=OPERATION_ENABLED, move=1, position=1000$'
Assert-Output $exercise13 '^Diagnostics: accepted=9, missing=3, rejected=1, watchdog_trips=0, published=13$'

# 在超时已锁存后加入一次合法长度、非法模式的报文，验证故障优先于停用状态。
$faultText = $text13.Replace('size_t length = now_ms == 9 ? MODE_PDO_SIZE - 1 : MODE_PDO_SIZE;', 'size_t length = MODE_PDO_SIZE;')
$variantFault = Write-Variant $faultText 'if (!ModePDO_PackRx(rx_bytes, sizeof rx_bytes, &master)) return 1;' "if (now_ms == 9) master.mode = 7;`r`n        if (!ModePDO_PackRx(rx_bytes, sizeof rx_bytes, &master)) return 1;" 'lesson13_invalid_mode'
$fault = Invoke-VerifiedProgram $variantFault 'lesson13_invalid_mode'
Assert-Output $fault '^t=9, valid=1, expired=0, latched=1, EC=SAFE-OP, drive=FAULT_REACTION_ACTIVE, move=0, position=700$'
Assert-Output $fault '^t=10, valid=1, expired=0, latched=1, EC=SAFE-OP, drive=FAULT, move=0, position=700$'
Assert-Output $fault '^t=12, valid=1, expired=0, latched=1, EC=SAFE-OP, drive=FAULT, move=0, position=700$'

if ((Get-FileHash -LiteralPath $source12 -Algorithm SHA256).Hash -ne $before12 -or
    (Get-FileHash -LiteralPath $source13 -Algorithm SHA256).Hash -ne $before13) {
    throw 'Original lesson sources changed during verification.'
}
Write-Output 'Integration checks passed: timeout before motion, recovery latch, both exercise answers, fault priority, original sources preserved'
$exercise12 | Select-String '^CSP tick=(2|10),'
$exercise13 | Select-String '^t=(6|12),|^Diagnostics:'
