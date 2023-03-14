@echo off
cl.exe /nologo /std:c17 /W4 /WX /Osy /EHsc /GL /c improg.c || exit /b 1
cl.exe /nologo /std:c17 /W4 /WX /Osy /EHsc /GL /c main.c || exit /b 1
cl.exe /nologo /W4 /WX /Osy /EHsc /GL /link /out:demo.exe improg.obj main.obj || exit /b 1
