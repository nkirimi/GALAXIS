@echo off
rem ======================================================================
rem  Galaxis build script for Windows (MinGW-w64: g++ and gcc on PATH)
rem
rem    build.bat cp1     CP1  technical foundation    -> galaxis_cp1.exe
rem    build.bat cp2     CP2  transformations + GIF   -> galaxis_cp2.exe
rem    build.bat cp3     CP3  OpenGL prototype        -> galaxis_cp3.exe
rem    build.bat all     all three (default)
rem    build.bat clean   delete build output
rem
rem  From the VS Code / PowerShell terminal type:  .\build.bat cp3
rem ======================================================================
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set TARGET=%1
if "%TARGET%"=="" set TARGET=all

where g++ >nul 2>nul
if errorlevel 1 (
    echo g++ was not found. Install MinGW-w64 - for example through MSYS2 - and add its bin folder to PATH.
    exit /b 1
)

set FLAGS=-std=c++17 -O2 -Iinclude
set CORE=
for %%f in (src\core\*.cpp) do set CORE=!CORE! %%f

if /i "%TARGET%"=="clean" (
    if exist build rmdir /s /q build
    del /q galaxis_cp1.exe galaxis_cp2.exe galaxis_cp3.exe 2>nul
    echo Cleaned.
    exit /b 0
)
if /i "%TARGET%"=="cp1" call :cp1
if /i "%TARGET%"=="cp2" call :cp2
if /i "%TARGET%"=="cp3" call :cp3
if /i "%TARGET%"=="all" (
    call :cp1
    if errorlevel 1 exit /b 1
    call :cp2
    if errorlevel 1 exit /b 1
    call :cp3
)
if errorlevel 1 (
    echo.
    echo Build FAILED.
    exit /b 1
)
exit /b 0

rem ----------------------------------------------------------------------
:cp1
set SRC=
for %%f in (src\cp1\*.cpp) do set SRC=!SRC! %%f
echo Building galaxis_cp1.exe ...
g++ %FLAGS% !CORE! !SRC! -o galaxis_cp1.exe -static
if errorlevel 1 exit /b 1
echo   done. Run: .\galaxis_cp1.exe   - images go to output\cp1
exit /b 0

rem ----------------------------------------------------------------------
:cp2
set SRC=
for %%f in (src\cp2\*.cpp) do set SRC=!SRC! %%f
echo Building galaxis_cp2.exe ...
g++ %FLAGS% !CORE! !SRC! -o galaxis_cp2.exe -static
if errorlevel 1 exit /b 1
echo   done. Run: .\galaxis_cp2.exe   - images and the GIF go to output\cp2
exit /b 0

rem ----------------------------------------------------------------------
:cp3
rem GLFW is compiled from the copy in third_party (C code, so gcc), once.
if not exist build\glfw mkdir build\glfw
echo Building GLFW (first time only) ...
for %%f in (context init input monitor platform vulkan window egl_context osmesa_context null_init null_monitor null_window null_joystick win32_init win32_joystick win32_monitor win32_time win32_thread win32_window win32_module wgl_context) do (
    if not exist build\glfw\%%f.o (
        gcc -c -O2 -D_GLFW_WIN32 -Ithird_party\glfw\include third_party\glfw\src\%%f.c -o build\glfw\%%f.o
        if errorlevel 1 exit /b 1
    )
)
set GLFW=
for %%f in (build\glfw\*.o) do set GLFW=!GLFW! %%f
set SRC=
for %%f in (src\cp3\*.cpp) do set SRC=!SRC! %%f
echo Building galaxis_cp3.exe ...
g++ %FLAGS% -Ithird_party\glad\include -Ithird_party\glfw\include !CORE! !SRC! !GLFW! -o galaxis_cp3.exe -static -lopengl32 -lgdi32
if errorlevel 1 exit /b 1
echo   done. Run: .\galaxis_cp3.exe
exit /b 0
