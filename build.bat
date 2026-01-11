@echo off
setlocal
cd /D "%~dp0"

:: Get cl.exe
where cl >nul 2>nul
if %errorlevel%==1 (
    echo Looking for 'vcvars64.bat'.. Recommended to run from the Developer Command Prompt.
    @call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)

where /q cl || (
    echo [ERROR]: "cl" not found - please run this from the MSVC x64 native tools command prompt.
    exit /b 1
)

if not exist build mkdir build
pushd build


:: Example 1. CLI ::
call cl /nologo /Od /Z7 /W4 /D_CRT_SECURE_NO_WARNINGS ..\examples\1_CLI\main.c /Fe:example_cli.exe /link /incremental:no /opt:ref

:: Example 2. Interactive ::
call cl /nologo /Od /Z7 /W4 /D_CRT_SECURE_NO_WARNINGS ..\examples\2_Interactive\main.c /Fe:example_interactive.exe /link /incremental:no /opt:ref glad.obj glfw3dll.lib glew32.lib

:: Example 3. Game ::
call cl /nologo /Od /std:c++14 /Z7 /W4 /D_CRT_SECURE_NO_WARNINGS ..\examples\3_Game\main.cpp /Fe:example_game.exe /link /incremental:no /opt:ref glad.obj glfw3dll.lib glew32.lib

popd
