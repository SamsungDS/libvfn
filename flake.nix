{
  description = "provides the libvfn package for NixOS and Nix environments";
	
	inputs = {
	  nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";

	};

	outputs = { self, nixpkgs }:
	  let
      # lists supported systems
		  allSystems = [
			  "x86_64-linux"
				"aarch64-linux"
			];

			forAllSystems = fn:
			  nixpkgs.lib.genAttrs allSystems
				(system: fn { pkgs = import nixpkgs { inherit system; };});
      
		in {
		  packages = forAllSystems ({ pkgs }: {
			  libvfn = pkgs.callPackage ./nix/packages/libvfn { };
			});
		};
}
