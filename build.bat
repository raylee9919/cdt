@echo off
setlocal
cd /D "%~dp0"

if not exist build mkdir build
pushd build

:: Example 1. Interactive ::
call cl /nologo /Od /Z7 /W4 /D_CRT_SECURE_NO_WARNINGS ..\examples\1_Interactive\main.c /Fe:example_interactive.exe /link /incremental:no /opt:ref glad.obj glfw3dll.lib glew32.lib

:: Example 2. Game ::
call cl /nologo /Od /Z7 /W4 /D_CRT_SECURE_NO_WARNINGS ..\examples\2_Game\main.c /Fe:example_game.exe /link /incremental:no /opt:ref glad.obj glfw3dll.lib glew32.lib

popd
