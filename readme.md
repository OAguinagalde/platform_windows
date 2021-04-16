# Working with the windows platform

Building is strightforward since there is no dependencys other than the windows provided libraries and APIs.


File: cl_build.ps1
```ps1
cl .\main.c /nologo opengl32.lib gdi32.lib user32.lib
```
File: clang_build.ps1
```ps1
clang .\main.c -l opengl32.lib -l user32.lib -l gdi32.lib
```