@echo off
cl.exe /NOLOGO /std:c17 /W4 /WX /Osy /EHsc /GL /c improg.c || exit /b 1
cl.exe /NOLOGO /std:c17 /W4 /WX /Osy /EHsc /GL /c main.c || exit /b 1
link.exe /NOLOGO /LTCG /WX /out:demo.exe improg.obj main.obj || exit /b 1
