param([string]$Cli = 'arduino-cli')
$ErrorActionPreference='Stop'
$localCli=Join-Path $PSScriptRoot '_validation\tools\arduino-cli\arduino-cli.exe'
$argsList=@('compile','--fqbn','arduino:avr:mega:cpu=atmega2560','--warnings','all','--build-path',(Join-Path $PSScriptRoot '_validation\mega-build'))
if($Cli -eq 'arduino-cli' -and (Test-Path -LiteralPath $localCli)) {
    $Cli=$localCli
    $argsList+=@('--config-file',(Join-Path $PSScriptRoot '_validation\arduino-cli.yaml'))
}
$argsList+=(Join-Path $PSScriptRoot 'firmware\FeedAxis')
& $Cli @argsList
if($LASTEXITCODE -ne 0) { throw 'Mega 2560 firmware compile failed. Install arduino:avr core first.' }
# Compile only. This script NEVER uploads to a board.
