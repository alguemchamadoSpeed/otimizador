@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSINSTALL="
set "MSBUILD="

rem Locate Visual Studio 2022 only (major version 17).
if exist "%VSWHERE%" (
    for /f "tokens=*" %%I in ('"%VSWHERE%" -latest -version "[17.0,18.0)" -products * -requires Microsoft.Component.MSBuild -property installationPath') do (
        set "VSINSTALL=%%I"
    )
)

rem The stable MSBuild path inside a Visual Studio 2022 installation.
if defined VSINSTALL (
    if exist "%VSINSTALL%\MSBuild\Current\Bin\MSBuild.exe" (
        set "MSBUILD=%VSINSTALL%\MSBuild\Current\Bin\MSBuild.exe"
    )
)

rem Fallback for Developer Command Prompt / custom MSBuild installations.
if not defined MSBUILD (
    for /f "delims=" %%I in ('where msbuild 2^>nul') do (
        set "MSBUILD=%%I"
        goto :msbuild_found
    )
)

:msbuild_found
if not defined MSBUILD (
    echo.
    echo [ERRO] Nao encontrei o MSBuild do Visual Studio 2022.
    echo.
    echo Verifique se o Visual Studio 2022 esta instalado e abra o
    echo Visual Studio Installer ^> Modificar ^> marque:
    echo   Desenvolvimento para desktop com C++
    echo.
    echo Em Componentes individuais, confirme:
    echo   - MSVC v143 - VS 2022 C++ x64/x86 build tools
    echo   - Windows 10 SDK ou Windows 11 SDK
    echo   - MSBuild
    echo.
    echo Depois clique em Modificar, aguarde a instalacao e execute este BAT novamente.
    goto :failed
)

rem Give a useful diagnostic if the IDE exists but the C++ compiler workload does not.
if defined VSINSTALL (
    if not exist "%VSINSTALL%\VC\Tools\MSVC" (
        echo [ERRO] O MSBuild foi encontrado, mas o compilador C++ MSVC nao esta instalado.
        echo Abra o Visual Studio Installer, modifique o VS 2022 e instale
        echo o workload "Desenvolvimento para desktop com C++".
        goto :failed
    )
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
