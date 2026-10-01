$ErrorActionPreference = 'Stop'
$sourceDirectory = Join-Path $PSScriptRoot 'vendor\SOEM'
$buildDirectory = Join-Path $PSScriptRoot 'build-official'

foreach ($tool in @('gcc', 'cmake', 'mingw32-make')) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "Required tool not found in PATH: $tool"
    }
}
if (-not (Test-Path -LiteralPath (Join-Path $sourceDirectory 'CMakeLists.txt'))) {
    throw 'SOEM source is missing. See README.md for the source download instructions.'
}

& cmake -S $sourceDirectory -B $buildDirectory -G 'MinGW Makefiles' -DSOEM_BUILD_SAMPLES=ON
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }

& cmake --build $buildDirectory --target slaveinfo simple_ng --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'SOEM build failed.' }

Write-Output 'SOEM tools built successfully. No network scan was performed.'
Write-Output (Join-Path $buildDirectory 'samples\slaveinfo\slaveinfo.exe')
Write-Output (Join-Path $buildDirectory 'samples\simple_ng\simple_ng.exe')
