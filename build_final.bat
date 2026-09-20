@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"
set "BUILD_DIRECTORY=%PROJECT_ROOT%\build-final"

cmake -S "%PROJECT_ROOT%" -B "%BUILD_DIRECTORY%" -G "Visual Studio 18 2026" -A x64 -DCMAKE_CONFIGURATION_TYPES=Release
if errorlevel 1 goto :failure

cmake --build "%BUILD_DIRECTORY%" --config Release --target Sandbox --parallel
if errorlevel 1 goto :failure

echo.
echo Final build is ready:
echo %BUILD_DIRECTORY%\sandbox\Release\Sandbox.exe
exit /b 0

:failure
echo.
echo Final build failed.
exit /b 1
