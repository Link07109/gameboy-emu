# gameboy-emu
Game Boy emulator written in C! (for fun)

## Build Instuctions:
This project was made on Linux. If you're using Windows, install WSL in order to build it.

1. Install nix using the [Determinate Nix Installer](https://github.com/DeterminateSystems/nix-installer) by running this command:
```sh
curl --proto '=https' --tlsv1.2 -sSf -L https://install.determinate.systems/nix | \
  sh -s -- install
```
2. Run this command to build the project:
```shell
nix develop
```

That's it!

## Usage:
Run this command to use the emulator:
```shell
./gameboy.out <path_to_rom_here>
```

## Credits:
### CPU and PPU:
I followed this [youtube series](https://www.youtube.com/playlist?list=PLVxiWMqQvhg_yk4qy2cSC3457wZJga_e5) for all the CPU and PPU code

### APU:
This [article](https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html), as well as the resources in the "Further Reading" section

This [repository](https://github.com/mmitch/gbsplay/blob/master/plugout_sdl.c) for the nanosleep code used for queueing audio
