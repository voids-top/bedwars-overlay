@echo off
REM Inject the newly built DLL without replacing locked util.dll
REM This searches for the running javaw.exe (Minecraft/Badlion) and injects ..\build\util.new.dll
for /f "tokens=2 delims=," %%a in ('tasklist /FI "IMAGENAME eq javaw.exe" /FO CSV /NH') do (
    echo injecting PID %%~a
    injector.exe %%~a ..\\build\\util.dll
)
