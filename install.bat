
SET KI_SCRIPT_DIR=%~dp0
set KI_PATH=c:\ki\
set KI_EXE=%KI_SCRIPT_DIR%ki.exe

IF NOT EXIST %KI_EXE% (echo "ki.exe not found, nothing to install"; EXIT /b)

mkdir KI_PATH;

For /F "Delims=" %%I In ('echo %PATH% ^| find /C /I "%KI_PATH%"') Do set KiPathExists=%%I 2>Nul
If %KiPathExists%==0 (setx /M path "%path%;%KI_PATH%")

cp %KI_SCRIPT_DIR%ki.exe %KI_PATH%ki.exe
cp %KI_SCRIPT_DIR%libcurl-x64.dll %KI_PATH%libcurl-x64.dll
