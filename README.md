# gameboy-emu
Game Boy emulator written in C! (for fun)

## Build Instuctions:
1. install nix
2. run `nix develop` to enter the development shell that has all the dependencies included
3. run `./build.sh` from within this development shell to build the project

## Usage:
run `./gameboy.out <path_to_rom_here>`

## Credits:
### CPU and PPU:
I followed this [youtube series](https://www.youtube.com/playlist?list=PLVxiWMqQvhg_yk4qy2cSC3457wZJga_e5) for all the CPU and PPU code

### APU:
This [article](https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html), as well as the resources in the "Further Reading" section

This [repository](https://github.com/mmitch/gbsplay/blob/master/plugout_sdl.c) for the nanosleep code used for queueing audio
