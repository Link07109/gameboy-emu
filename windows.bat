@echo off

gcc src/main.c -o gameboy.exe -I C:\SDL2-devel\include -L C:\SDL2-devel\lib -lSDL2 -mthreads