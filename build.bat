@echo off
setlocal
cd /D "%~dp0"

if not exist build mkdir build
pushd build

set SRC=..\examples\demo.cpp 
call cl -std:c++17 -I..\src\vendor /nologo /Od /Z7 /W4 /EHsc- /D_CRT_SECURE_NO_WARNINGS -wd4201 -wd4456 -wd4477 -wd4189 -wd4100 %SRC% -Fe:demo.exe /link /incremental:no /opt:ref glad.obj glfw3dll.lib glew32.lib

popd
