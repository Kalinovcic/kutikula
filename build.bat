@echo off
cl /nologo /O2 /DEBUG /Fo"run_tree/kutikula.obj" /Fe"run_tree/kutikula.exe" /MT /EHsc src/kutikula.cpp /link /nologo /subsystem:console kernel32.lib user32.lib gdi32.lib opengl32.lib advapi32.lib lib/SDL2.lib lib/glew32.lib
