param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$Interface,

    [ValidateSet('None', 'Sdo', 'Mapping')]
    [string]$Details = 'None'
)

$ErrorActionPreference = 'Stop'
$executable = Join-Path $PSScriptRoot 'build-official\samples\slaveinfo\slaveinfo.exe'
$npcapDirectory = Join-Path $env:WINDIR 'System32\Npcap'

if (-not (Test-Path -LiteralPath $executable)) {
    throw 'slaveinfo.exe is missing. Run .\EtherCAT_Master_Lab\build.ps1 first.'
}
if (-not $Interface.StartsWith('\Device\NPF_{', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Use the wired adapter Npcap name printed by list-adapters.ps1, such as \Device\NPF_{GUID}.'
}

$arguments = @($Interface)
switch ($Details) {
    'Sdo' { $arguments += '-sdo' }
    'Mapping' { $arguments += '-map' }
}

$previousPath = $env:PATH
try {
    if (Test-Path -LiteralPath $npcapDirectory) {
        $env:PATH = $npcapDirectory + ';' + $previousPath
    }
    Write-Output "Scanning EtherCAT slaves on: $Interface"
    & $executable @arguments
    if ($LASTEXITCODE -ne 0) { throw "slaveinfo exited with code $LASTEXITCODE" }
    # The official sample can return 0 even when no slave was found.
    Write-Output 'Check the reported slave count and state; an exit code of 0 alone is not a communication pass.'
}
finally {
    $env:PATH = $previousPath
}
