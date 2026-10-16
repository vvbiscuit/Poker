@echo off
rem ---------------------------------------------------------------------------
rem Build script for the Texas Holdem simulation.
rem
rem   build.cmd          optimized build, writes texas_holdem.exe
rem   build.cmd debug    build with debug info, writes build\Debug\outDebug.exe
rem   build.cmd run      optimized build, then runs the program
rem   build.cmd clean    removes the build outputs
rem
rem The compiler is g++ or clang++ in C++17 mode. It is taken from PATH first,
rem then from the usual MinGW-w64 and LLVM installation folders. MSVC is not
rem used here; see ReadMe.txt for the cl command line.
rem ---------------------------------------------------------------------------

setlocal
cd /d "%~dp0"

set "SOURCE=texas_holdem.cpp"
set "RELEASE_EXE=texas_holdem.exe"
set "DEBUG_DIR=build\Debug"
set "DEBUG_EXE=%DEBUG_DIR%\outDebug.exe"
set "WARNINGS=-Wall -Wextra"

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=release"

if /i "%TARGET%"=="clean" goto clean
if /i "%TARGET%"=="release" goto chooseCompiler
if /i "%TARGET%"=="debug" goto chooseCompiler
if /i "%TARGET%"=="run" goto chooseCompiler

echo Unknown option: %TARGET%
echo Usage: build.cmd [release ^| debug ^| run ^| clean]
exit /b 2

rem ---------------------------------------------------------------------------
:clean
echo Removing build outputs
if exist "%RELEASE_EXE%" del /q "%RELEASE_EXE%"
if exist "%DEBUG_EXE%" del /q "%DEBUG_EXE%"
if exist "%DEBUG_DIR%" rd /q "%DEBUG_DIR%" 2>nul
if exist "build" rd /q "build" 2>nul
echo Done.
exit /b 0

rem ---------------------------------------------------------------------------
:chooseCompiler
if not exist "%SOURCE%" (
    echo Error: "%SOURCE%" was not found next to this script.
    exit /b 1
)

set "CXX="
where g++ >nul 2>nul
if not errorlevel 1 call :useCompiler g++
if not defined CXX if exist "D:\mingw64\bin\g++.exe" call :useCompiler "D:\mingw64\bin\g++.exe"
if not defined CXX if exist "C:\mingw64\bin\g++.exe" call :useCompiler "C:\mingw64\bin\g++.exe"
if not defined CXX if exist "C:\msys64\mingw64\bin\g++.exe" call :useCompiler "C:\msys64\mingw64\bin\g++.exe"
where clang++ >nul 2>nul
if not defined CXX if not errorlevel 1 call :useCompiler clang++
if not defined CXX if exist "C:\Program Files\LLVM\bin\clang++.exe" call :useCompiler "C:\Program Files\LLVM\bin\clang++.exe"

if not defined CXX (
    echo Error: no C++ compiler found.
    echo Install MinGW-w64 or LLVM and put g++ or clang++ on PATH.
    exit /b 1
)

if /i "%TARGET%"=="debug" goto buildDebug
goto buildRelease

rem ---------------------------------------------------------------------------
:buildRelease
echo Building %SOURCE% with %CXX%
"%CXX%" -std=c++17 %WARNINGS% -O2 -o "%RELEASE_EXE%" "%SOURCE%"
if errorlevel 1 goto buildFailed
for %%F in ("%RELEASE_EXE%") do echo Build succeeded: %%~fF  ^(%%~zF bytes^)
if /i "%TARGET%"=="run" goto runIt
exit /b 0

:runIt
echo Running %RELEASE_EXE%
"%RELEASE_EXE%"
exit /b %ERRORLEVEL%

rem ---------------------------------------------------------------------------
:buildDebug
if not exist "%DEBUG_DIR%" mkdir "%DEBUG_DIR%"
echo Building %SOURCE% with %CXX% ^(debug^)
"%CXX%" -std=c++17 %WARNINGS% -g -O0 -o "%DEBUG_EXE%" "%SOURCE%"
if errorlevel 1 goto buildFailed
for %%F in ("%DEBUG_EXE%") do echo Build succeeded: %%~fF  ^(%%~zF bytes^)
exit /b 0

rem ---------------------------------------------------------------------------
:buildFailed
echo Build failed.
exit /b 1

:useCompiler
set "CXX=%~1"
exit /b 0