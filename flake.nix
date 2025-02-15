{
  description = "Uses clang to build the project";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs }: 
  let
    pkgs = nixpkgs.legacyPackages."x86_64-linux";
  in
  {
    devShells."x86_64-linux".default = with pkgs;
    clangStdenv.mkDerivation {
      name = "clang-nix-shell";
      buildInputs = [ SDL2 ];

      shellHook = ''
        clang -std=c17 src/main.c -o gameboy.out -Wall -ISDL2 -lSDL2 -pthread -ggdb3
        exit
      '';
    };
  };
}
