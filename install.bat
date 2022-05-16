@echo off

set KI_PATH=%~dp0\

echo ;%PATH%; | find /C /I ";%KI_PATH%;" > nul
if not errorlevel 1 goto jump
set PATH=%PATH%;%KI_PATH%
setx /M PATH "%PATH%;%KI_PATH%"
:jump
