@echo off
setlocal
rem Compatibilidade com quem ainda inicia pelo antigo build.bat.
call "%~dp0Otimizador.bat"
exit /b %ERRORLEVEL%
