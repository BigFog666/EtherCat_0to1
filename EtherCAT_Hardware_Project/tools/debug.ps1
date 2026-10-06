. (Join-Path $PSScriptRoot 'environment.ps1')
Push-Location $ProjectRoot
try {
    $gdb = Join-Path $ToolPaths.arm_root 'bin/arm-none-eabi-gdb.exe'
    Invoke-Checked $gdb @('-x','config/debug.gdb')
} finally { Pop-Location }
