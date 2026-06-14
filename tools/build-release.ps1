$ErrorActionPreference = "Stop"

cmake --preset msvc-release
cmake --build --preset release

$exe = Join-Path $PSScriptRoot "..\build\release\Restime.exe"
if (Test-Path $exe) {
    Get-FileHash $exe -Algorithm SHA256 | Format-List
}
