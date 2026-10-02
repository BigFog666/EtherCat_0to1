$ErrorActionPreference = 'Stop'
# 使用脚本所在目录定位文件，因此从其他目录调用本脚本也可以构建。
$outputDirectory = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$executable = Join-Path $outputDirectory 'ethercat_sim.exe'
# 编译本课所需 C 文件，生成可执行程序。用数组存放参数，避免反引号续行。
# -std=c11 使用 C11；-Wall 等选项打开编译警告，-Werror 将警告视为错误。
$compilerArguments = @(
    '-std=c11'
    '-Wall'
    '-Wextra'
    '-Wpedantic'
    '-Werror'
    (Join-Path $PSScriptRoot 'main.c')
    (Join-Path $PSScriptRoot 'EtherCAT\ethercat_state.c')
    (Join-Path $PSScriptRoot 'EtherCAT\ethercat_pdo.c')
    (Join-Path $PSScriptRoot 'EtherCAT\ethercat_od.c')
    (Join-Path $PSScriptRoot 'EtherCAT\ethercat_sdo.c')
    '-o'
    $executable
)
# @compilerArguments 将数组中的每一项作为一个独立参数传给 GCC。
& gcc @compilerArguments
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed.' }
# 编译成功后立即运行新程序；编译失败则停下，不运行旧程序。
& $executable
if ($LASTEXITCODE -ne 0) { throw 'Simulator failed.' }
