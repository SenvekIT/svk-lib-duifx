<#
.SYNOPSIS
    Configure, build and test DUIFx for C++ on Windows (MSVC + Qt).

.EXAMPLE
    .\cpp\scripts\build.ps1                      # Debug build + tests
    .\cpp\scripts\build.ps1 -Config release      # Release build + tests
    .\cpp\scripts\build.ps1 -BuildDir build\mine # own build folder (parallel builds)
    .\cpp\scripts\build.ps1 -QtDir C:\Qt\6.8.3\msvc2022_64 -NoTests
#>
param(
    [ValidateSet("debug", "release")] [string] $Config = "debug",
    [string] $QtDir = $env:QTDIR,
    [string] $BuildDir,
    [switch] $NoTests,
    [switch] $Clean
)
$ErrorActionPreference = "Stop"

# Qt: the newest msvc2022_64 kit under C:\Qt with Qt SVG unless given.
if (-not $QtDir) {
    $kit = Get-ChildItem "C:\Qt" -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' -and (Test-Path "$($_.FullName)\msvc2022_64\lib\cmake\Qt6Svg") } |
        Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
    if (-not $kit) { throw "No Qt with Qt SVG found under C:\Qt - pass -QtDir" }
    $QtDir = "$($kit.FullName)\msvc2022_64"
}
$env:QTDIR = $QtDir

# MSVC developer environment (newest Visual Studio with C++ tools).
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vs = & $vswhere -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vs) { throw "No Visual Studio with the C++ tools found" }
    Import-Module "$vs\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
    Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64" | Out-Null
}

# CMake and Ninja from the Qt installation if not on PATH.
foreach ($tool in @("C:\Qt\Tools\CMake_64\bin", "C:\Qt\Tools\Ninja")) {
    if (Test-Path $tool) { $env:PATH = "$tool;$env:PATH" }
}
$env:PATH = "$QtDir\bin;$env:PATH"

$root = Split-Path $PSScriptRoot -Parent
$preset = "windows-msvc-$Config"
if (-not $BuildDir) { $BuildDir = "build\$preset" }
# From here on only exit codes count: CMake and Ninja print warnings on stderr,
# which Windows PowerShell 5.1 would turn into terminating errors.
$ErrorActionPreference = "Continue"
Push-Location $root
try {
    if ($Clean) { Remove-Item -Recurse -Force $BuildDir -ErrorAction SilentlyContinue }
    Write-Host "Qt: $QtDir" -ForegroundColor Cyan
    cmake --preset $preset -B $BuildDir
    if ($LASTEXITCODE) { throw "configure failed" }
    cmake --build $BuildDir
    if ($LASTEXITCODE) { throw "build failed" }
    if (-not $NoTests) {
        ctest --test-dir $BuildDir --output-on-failure
        if ($LASTEXITCODE) { throw "tests failed" }
    }
    Write-Host "OK: $BuildDir" -ForegroundColor Green
} finally {
    Pop-Location
}
