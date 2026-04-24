@echo off
setlocal EnableDelayedExpansion

if "%EMSDK%" == "" (
    echo [ERROR] EMSDK environment not activated.
    echo Run: emsdk_env.bat  from your emsdk directory
    exit /b 1
)

if not exist build mkdir build

echo [1/2] Configuring...
call emcmake cmake -B build -S . -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

echo [2/2] Building...
cd build
ninja
if errorlevel 1 (
    cd ..
    echo [ERROR] Build failed.
    exit /b 1
)
cd ..

echo.
echo Build successful.
echo Output: web\prime_module.js + web\prime_module.wasm
echo.
echo Start dev server:  python web\server.py
echo Then open:         http://localhost:8080
pause