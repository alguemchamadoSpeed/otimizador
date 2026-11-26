@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "MSBUILD="

if exist "%VSWHERE%" (
    for /f "tokens=*" %%i in ('"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe') do (
        set "MSBUILD=%%i"
    )
)

if not defined MSBUILD (
    for /f "delims=" %%i in ('where msbuild 2^>nul') do (
        set "MSBUILD=%%i"
        goto :found
    )
)

:found
if not defined MSBUILD (
    echo [ERRO] MSBuild nao foi encontrado.
    echo Instale o Visual Studio 2022 com o workload Desenvolvimento para desktop com C++.
    goto :failed
)

echo.
echo ==========================================
echo       OTIMIZADOR - BUILD WINDOWS x64
echo ==========================================
echo Usando: "%MSBUILD%"
echo.

"%MSBUILD%" "%ROOT%Otimizador.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64 /v:minimal
if errorlevel 1 goto :failed

echo.
echo [OK] Build concluido.
echo Executavel: "%ROOT%bin\x64\Release\Otimizador.exe"
if /I not "%CI%"=="true" pause
exit /b 0

:failed
echo.
echo [FALHA] A compilacao nao foi concluida.
if /I not "%CI%"=="true" pause
exit /b 1
