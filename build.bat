@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSINSTALL="
set "MSBUILD="
set "NUGET="

rem Locate Visual Studio 2022 only (major version 17).
if exist "%VSWHERE%" (
    for /f "tokens=*" %%I in ('"%VSWHERE%" -latest -version "[17.0,18.0)" -products * -requires Microsoft.Component.MSBuild -property installationPath') do (
        set "VSINSTALL=%%I"
    )
)

if defined VSINSTALL (
    if exist "%VSINSTALL%\MSBuild\Current\Bin\MSBuild.exe" (
        set "MSBUILD=%VSINSTALL%\MSBuild\Current\Bin\MSBuild.exe"
    )
)

if not defined MSBUILD (
    echo.
    echo [ERRO] Nao encontrei o MSBuild do Visual Studio 2022.
    echo Instale VS 2022 com Desenvolvimento para desktop com C++.
    echo Componentes: MSVC v143, Windows SDK e MSBuild.
    goto :failed
)

if not exist "%VSINSTALL%\VC\Tools\MSVC" (
    echo.
    echo [ERRO] O compilador MSVC C++ v143 nao esta instalado.
    echo Abra Visual Studio Installer ^> Modificar ^> Desenvolvimento para desktop com C++.
    goto :failed
)

where node >nul 2>nul
if errorlevel 1 (
    echo.
    echo [ERRO] Node.js nao foi encontrado.
    echo Instale Node.js 20 ou superior, incluindo npm, e abra um novo terminal.
    goto :failed
)

where npm.cmd >nul 2>nul
if errorlevel 1 (
    echo [ERRO] npm nao foi encontrado. Reinstale Node.js incluindo npm.
    goto :failed
)

rem Find NuGet CLI. If absent, download the official command-line client to TEMP.
if exist "%ProgramFiles(x86)%\NuGet\nuget.exe" set "NUGET=%ProgramFiles(x86)%\NuGet\nuget.exe"
if not defined NUGET (
    for /f "delims=" %%I in ('where nuget.exe 2^>nul') do (
        set "NUGET=%%I"
        goto :nuget_found
    )
)

:nuget_found
if not defined NUGET (
    set "NUGET=%TEMP%\otimizador-nuget.exe"
    if not exist "%NUGET%" (
        echo Baixando NuGet CLI oficial...
        powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Invoke-WebRequest -UseBasicParsing -Uri 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe' -OutFile '%NUGET%'"
        if errorlevel 1 (
            echo [ERRO] Nao foi possivel baixar o NuGet CLI.
            goto :failed
        )
    )
)

echo.
echo ==========================================
echo       OTIMIZADOR - BUILD WINDOWS x64
echo ==========================================
echo MSBuild: "%MSBUILD%"
echo Node:
node --version
echo.

echo Restaurando WebView2 pelo NuGet...
"%NUGET%" restore "%ROOT%Otimizador.sln" -PackagesDirectory "%ROOT%packages" -NonInteractive
if errorlevel 1 goto :failed

echo.
echo Compilando host C++ e interface React...
"%MSBUILD%" "%ROOT%Otimizador.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64 /v:minimal
if errorlevel 1 goto :failed

echo.
echo [OK] Build concluido.
echo Executavel: "%ROOT%bin\x64\Release\Otimizador.exe"
echo Interface:  "%ROOT%bin\x64\Release\web"
if /I not "%CI%"=="true" pause
exit /b 0

:failed
echo.
echo [FALHA] A compilacao nao foi concluida.
if /I not "%CI%"=="true" pause
exit /b 1
