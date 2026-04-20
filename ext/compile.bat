@echo off
setlocal
pushd %~dp0
if not exist ..\build mkdir ..\build
cl /EHsc /O2 /utf-8 injector.cpp
if errorlevel 1 exit /b %errorlevel%
copy /Y injector.exe ..\injector.exe >nul
if exist "%JAVA_HOME%\bin\javac.exe" (
  "%JAVA_HOME%\bin\javac.exe" --release 8 -d ..\build AgentRunnable.java
  if errorlevel 1 exit /b %errorlevel%
  copy /Y ..\build\AgentRunnable.class ..\AgentRunnable.class >nul
)
cl /LD /utf-8 /EHsc util.cpp /I"%JAVA_HOME%\include" /I"%JAVA_HOME%\include\win32" Ws2_32.lib "%JAVA_HOME%\lib\jvm.lib" /link /OUT:..\build\util-stage.dll
if errorlevel 1 exit /b %errorlevel%
copy /Y ..\build\util-stage.dll ..\build\util.dll >nul
copy /Y ..\build\util-stage.dll ..\util.dll >nul
del /Q ..\build\util-stage.dll >nul 2>nul
echo Build complete: ..\build\util.dll
popd
endlocal
