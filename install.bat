@echo off

set KI_SCRIPT_DIR=%~dp0
set KI_PATH=c:\ki\
set KI_EXE=%KI_SCRIPT_DIR%ki.exe

IF NOT EXIST %KI_EXE% (echo "ki.exe not found, nothing to install"; EXIT /b)

IF NOT EXIST %KI_PATH% (mkdir %KI_PATH%)

echo ;%PATH%; | find /C /I ";%KI_PATH%;" > nul
if not errorlevel 1 goto jump
set PATH=%PATH%;%KI_PATH%
setx /M PATH "%PATH%;%KI_PATH%"
:jump

cp %KI_SCRIPT_DIR%ki.exe %KI_PATH%ki.exe
cp %KI_SCRIPT_DIR%libcurl-x64.dll %KI_PATH%libcurl-x64.dll
