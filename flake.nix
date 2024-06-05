{
  description = "provides the libvfn package for NixOS and Nix environments";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";

  };

  outputs = { self, nixpkgs }:
    let
      # lists supported systems
      allSystems = [ "x86_64-linux" "aarch64-linux" ];

      forAllSystems = fn:
        nixpkgs.lib.genAttrs allSystems
        (system: fn { pkgs = import nixpkgs { inherit system; }; });

    in {
      formatter = forAllSystems ({ pkgs }: pkgs.nixfmt);
      packages = forAllSystems ({ pkgs }: rec {
        libvfn = pkgs.stdenv.mkDerivation {
          pname = "libvfn";
          version = "4.0.1";
          src = ./.;
          mesonFlags = [
            "-Ddocs=disabled"
            "-Dlibnvme=disabled"
            "-Dprofiling=false"
          ];
          nativeBuildInputs = with pkgs; [ meson ninja pkg-config perl ];
        };
        default = libvfn;
      });
    };
}
