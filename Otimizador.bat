@echo off
setlocal EnableExtensions

title Otimizador - Windows 10/11

set "SCRIPT=%~dp0Otimizador.ps1"
set "POWERSHELL=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"

if not exist "%SCRIPT%" (
    echo.
    echo [ERRO] Otimizador.ps1 nao encontrado.
    echo Coloque este BAT na mesma pasta do arquivo PS1.
    echo.
    pause
    exit /b 1
)

if not exist "%POWERSHELL%" set "POWERSHELL=powershell.exe"

"%POWERSHELL%" -NoLogo -NoProfile -ExecutionPolicy Bypass -STA -File "%SCRIPT%"
set "EXIT_CODE=%ERRORLEVEL%"

if not "%EXIT_CODE%"=="0" (
    echo.
    echo [ERRO] O Otimizador foi encerrado com codigo %EXIT_CODE%.
    pause
)

exit /b %EXIT_CODE%
