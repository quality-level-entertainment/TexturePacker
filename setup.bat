@echo off
title "CMake Setup"

set buildDir=build
set cmakeDir=CMake

@REM Skip function definition so they won't get called
goto :main

@REM Function definition for all actions

:Build
    echo === Creating CMake.MSbuild folders
    if NOT exist "%cmakeDir%" mkdir "%cmakeDir%"
    @REM if NOT exist "%buildDir%/msbuild" mkdir "%buildDir%/msbuild"

    echo === Creating project with msbuild
    cd "%cmakeDir%"
    cmake ..

    echo === Building project
    cmake --build .

    echo === Building complete
    cd ..
    echo.
exit /b
:Clean
    @REM Delete the entire build folder
    if exist %buildDir% (
        echo === Cleaning build directory...
        echo Deleting %buildDir%
        rmdir /S /Q %buildDir%
    )
    if exist %cmakeDir% (
        echo === Cleaning CMake directory...
        echo Deleting %cmakeDir%
        rmdir /S /Q %cmakeDir%
    )
    echo === Cleaning complete
    echo.
exit /b

@REM Main logic start
:main

echo.

@REM If no arguments are passed, it means we will build the program
if "%1" == "" (
    call :Build
    exit /b 0
)

if "%1" == "build" (
    call :Build
    exit /b 0
)

if "%1" == "clean" (
    call :Clean
    exit /b 0
)

echo Error: Invalid arguments.
echo Please use 'build' or 'clean'.