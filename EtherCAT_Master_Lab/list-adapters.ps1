$ErrorActionPreference = 'Stop'
$executable = Join-Path $PSScriptRoot 'build-official\samples\slaveinfo\slaveinfo.exe'
$npcapDirectory = Join-Path $env:WINDIR 'System32\Npcap'

if (-not (Test-Path -LiteralPath $executable)) {
    throw 'slaveinfo.exe is missing. Run .\EtherCAT_Master_Lab\build.ps1 first.'
}

# Prefer the installed Npcap DLLs without changing the system PATH.
$previousPath = $env:PATH
try {
    if (Test-Path -LiteralPath $npcapDirectory) {
        $env:PATH = $npcapDirectory + ';' + $previousPath
    }
    # With no arguments the official sample only enumerates interfaces.
    & $executable
    if ($LASTEXITCODE -ne 0) { throw "Adapter enumeration failed: exit code $LASTEXITCODE" }
}
finally {
    $env:PATH = $previousPath
}
