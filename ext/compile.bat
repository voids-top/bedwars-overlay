@echo off
setlocal
pushd %~dp0
if not exist ..\build mkdir ..\build
cl /EHsc /O2 /utf-8 injector.cpp
if errorlevel 1 exit /b %errorlevel%
copy /Y injector.exe ..\injector.exe >nul
cl /LD /utf-8 /EHsc util.cpp /I"%JAVA_HOME%\include" /I"%JAVA_HOME%\include\win32" Ws2_32.lib "%JAVA_HOME%\lib\jvm.lib" /link /OUT:..\build\util8b.dll
if errorlevel 1 exit /b %errorlevel%
echo Build complete: ..\build\util8b.dll
popd
endlocal
