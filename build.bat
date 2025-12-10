@echo off
setlocal
cd /D "%~dp0"

if not exist build mkdir build
pushd build

call cl /I..\src\vendor /nologo /Od /Z7 /W4 /D_CRT_SECURE_NO_WARNINGS ..\example\ex_main.c -Fe:example.exe /link /incremental:no /opt:ref glad.obj glfw3dll.lib glew32.lib
popd
