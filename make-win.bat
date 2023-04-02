@echo off
if not exist "%~dp0build" md "%~dp0build"
cl.exe /nologo /std:c17 /W4 /WX /Osy /EHsc /GL /c "%~dp0improg.c"  /Fo"%~dp0build\improg.obj" || exit /b 1
cl.exe /nologo /std:c17 /W4 /WX /Osy /EHsc /GL /c "%~dp0main.c" /Fo"%~dp0build\main.obj" || exit /b 1
link.exe /NOLOGO /LTCG /WX /out:"%~dp0build\demo.exe" "%~dp0build\improg.obj" "%~dp0build\main.obj" || exit /b 1
