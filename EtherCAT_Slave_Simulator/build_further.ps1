param(
    [ValidateSet('12')]
    [string]$Lesson = '12'
)
$ErrorActionPreference = 'Stop'
# 后续课程单独构建，旧 build.ps1 和 main.c 的练习继续保留。
$outputDirectory = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$executable = Join-Path $outputDirectory ('lesson' + $Lesson + '.exe')
$compilerArguments = @(
    '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror'
    (Join-Path $PSScriptRoot ('Further_Lessons/lesson' + $Lesson + '.c'))
    (Join-Path $PSScriptRoot 'Further_Lessons/joint.c')
    (Join-Path $PSScriptRoot 'Further_Lessons/mode_pdo.c')
    (Join-Path $PSScriptRoot 'EtherCAT/ethercat_pdo.c')
    (Join-Path $PSScriptRoot 'EtherCAT/ethercat_state.c')
    (Join-Path $PSScriptRoot 'CiA402/cia402.c')
    (Join-Path $PSScriptRoot 'CiA402/cia402_state.c')
    (Join-Path $PSScriptRoot 'CiA402/cia402_fault.c')
    '-o', $executable
)
& gcc @compilerArguments
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed.' }
& $executable
if ($LASTEXITCODE -ne 0) { throw 'Lesson example failed.' }
