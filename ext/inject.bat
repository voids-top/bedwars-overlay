for /f "tokens=2 delims=," %%a in ('tasklist /FI "IMAGENAME eq javaw.exe" /FO CSV /NH') do (
    injector.exe %%a util.dll
)
pause