@echo off
if not exist "%~dp0build" md "%~dp0build"
cl.exe /nologo /Z7 /std:c17 /W4 /WX /Osy /EHsc /GL /c "%~dp0improg.c" /I include /Fo"%~dp0build\improg.obj" || exit /b 1
cl.exe /nologo /Z7 /std:c17 /W4 /WX /Osy /EHsc /GL /c "%~dp0examples\improg-demo.c" /I include /Fo"%~dp0build\improg-demo.obj" || exit /b 1
link.exe /NOLOGO /DEBUG:FULL /LTCG /WX /out:"%~dp0build\demo.exe" "%~dp0build\improg.obj" "%~dp0build\improg-demo.obj" || exit /b 1
