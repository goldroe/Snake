@echo off

IF NOT EXIST build MKDIR build
PUSHD build

CL -nologo -FC -Zi ..\code\snake.cpp ..\ext\glad\src\glad.c -I ..\ext -I ..\ext\SDL\include -I ..\ext\glad\include -link -SUBSYSTEM:CONSOLE -LIBPATH:..\ext\SDL\lib\x64\ SDL2.lib SDL2main.lib shell32.lib

COPY *.exe ..
POPD
