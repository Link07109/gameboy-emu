# gameboy-emu
Game Boy emulator written in C! (for fun)

## Linux
### Build Instuctions:
These steps also work for WSL (Windows Subsystem for Linux)

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

### Usage:
Run this command to use the emulator:
```shell
./gameboy.out <path_to_rom_here>
```

## Windows
### Build Instuctions:
1. Install MINGW64
2. Download `SDL2-devel-2.32.0-mingw.zip` from the [SDL repository](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.0) and extract it
3. Copy the `x86_64-w64-mingw32` folder anywhere you want and rename it to `SDL2-devel`. I put mine in my C: directory.
4. Edit the `windows.bat` file to point to the folder mentioned above
5. Copy the `SDL2.dll` file from the `bin` subdirectory of the folder mentioned above and put it into the project root
6. Run the `windows.bat` file

The exe should now be built!

### Usage:
Run this command to use the emulator:
```shell
.\gameboy.exe <path_to_rom_here>
```

## Credits:
### CPU and PPU:
I followed this [youtube series](https://www.youtube.com/playlist?list=PLVxiWMqQvhg_yk4qy2cSC3457wZJga_e5) for all the CPU and PPU code

### APU:
This [article](https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html), as well as the resources in the "Further Reading" section

This [repository](https://github.com/mmitch/gbsplay/blob/master/plugout_sdl.c) for the nanosleep code used for queueing audio
