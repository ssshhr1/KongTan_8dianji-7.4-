param([ValidateSet('Debug','Release')][string]$Configuration = 'Release', [switch]$Test)
$ErrorActionPreference = 'Stop'
$locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $locator)) { throw 'Visual Studio Installer / vswhere.exe not found.' }
$builder = & $locator -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (!$builder) { throw 'Visual Studio MSBuild not found.' }
Push-Location $PSScriptRoot
try {
    & $builder 'KongTan_8dianji.vcxproj' /t:Build "/p:Configuration=$Configuration" /p:Platform=x64 /m /nologo /v:minimal
    if ($LASTEXITCODE -ne 0) { throw 'Application build failed.' }
    if ($Test) {
        & $builder 'tests\controller_tests.vcxproj' /t:Build /p:Configuration=Debug /p:Platform=x64 /m /nologo /v:minimal
        if ($LASTEXITCODE -ne 0) { throw 'Test build failed.' }
        & '.\tests\bin\controller_tests.exe'
        if ($LASTEXITCODE -ne 0) { throw 'Offline tests failed.' }
    }
} finally { Pop-Location }
