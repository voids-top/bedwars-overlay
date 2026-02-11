@echo off
setlocal
pushd %~dp0
for /f "tokens=2 delims=," %%a in ('tasklist /FI "IMAGENAME eq javaw.exe" /FO CSV /NH') do (
    echo injecting PID %%~a
injector.exe %%~a ..\build\util8b.dll
)
popd
endlocal
