@echo off
py -3.10 -m PyInstaller overlay.spec
if errorlevel 1 exit /b %errorlevel%

if /I "%CI%"=="true" exit /b 0

pause
