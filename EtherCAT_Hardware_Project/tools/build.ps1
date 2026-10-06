param([ValidateSet('All','Firmware','Host')][string]$Target = 'All')
. (Join-Path $PSScriptRoot 'environment.ps1')
$OldPath = $env:PATH
Push-Location $ProjectRoot
try {
    $env:PATH = (Split-Path -Parent $ToolPaths.make) + ';' + $env:PATH
    Invoke-Checked $ToolPaths.python @('tools/prepare_firmware.py')
    Invoke-Checked $ToolPaths.python @('tools/generate_sii.py')
    if ($Target -eq 'All' -or $Target -eq 'Firmware') {
        $configure = @('-S','.', '-B','build/firmware-gcc','-G','MinGW Makefiles', "-DCMAKE_MAKE_PROGRAM=$($ToolPaths.make)", '-DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc.cmake', "-DARM_GCC_ROOT=$($ToolPaths.arm_root)", '-DPROJECT_FIRMWARE=ON')
        Invoke-Checked $ToolPaths.cmake $configure
        Invoke-Checked $ToolPaths.cmake @('--build','build/firmware-gcc','--parallel','8')
    }
    if ($Target -eq 'All' -or $Target -eq 'Host') {
        $configure = @('-S','.', '-B','build/host','-G','MinGW Makefiles', "-DCMAKE_MAKE_PROGRAM=$($ToolPaths.make)", "-DCMAKE_C_COMPILER=$($ToolPaths.host_gcc)", '-DPROJECT_FIRMWARE=OFF','-DCMAKE_BUILD_TYPE=Debug')
        if (Test-Path -LiteralPath 'vendor/SOEM/CMakeLists.txt') { $configure += "-DSOEM_SOURCE_DIR=$($ProjectRoot.Replace('\','/'))/vendor/SOEM" }
        Invoke-Checked $ToolPaths.cmake $configure
        Invoke-Checked $ToolPaths.cmake @('--build','build/host','--parallel','8')
        $ctest = Join-Path (Split-Path -Parent $ToolPaths.cmake) 'ctest.exe'
        Invoke-Checked $ctest @('--test-dir','build/host','--output-on-failure')
    }
    Invoke-Checked $ToolPaths.python @('tests/check_config.py')
    Write-Host '本地构建完成；本次构建不访问开发板，不烧录、不写 EEPROM。'
} finally { $env:PATH = $OldPath; Pop-Location }
