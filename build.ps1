if (!(Test-Path "bin")) { New-Item "bin" -Type Directory }
cl main.c /Zi /Fe".\bin\main.exe" /nologo /link opengl32.lib user32.lib gdi32.lib