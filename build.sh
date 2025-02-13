clang -std=c17 src/main.c -o gameboy.out -Wall -I/usr/include/SDL2 -L/usr/libx86_64-linux-gnu -lSDL2 -lSDL2_ttf -lm -D_POSIX_C_SOURCE=199309L -ggdb3
