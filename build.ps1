param (
    [Switch]$Release
)
if (!(Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw [Exception]::new("cl.exe is not in path!")
}
if (!(Test-Path "bin")) { $null = New-Item "bin" -Type Directory }
if (!(Test-Path "obj")) { $null = New-Item "obj" -Type Directory }
# Zi for debug information
# Fe for executable output path
# Fo for intermediate objects path
# Fd for debugging files path
# nologo for no logo, duh
# 0d for disabling optimizations
# 02 for fastest optimizations
if ($Release) {
    cl win32.c win32_opengl.c /WX /Wall /Zi /Fe".\bin\win32.exe" /Fo".\obj\" /Fd".\obj\" /nologo /O2
}
else {
    cl win32.c win32_opengl.c /WX /W4 /Zi /Fe".\bin\win32.exe" /Fo".\obj\" /Fd".\obj\" /nologo /Od
}

if ($?) {
    Write-Host "OK" -ForeGroundColor Green
}