{
  description = "FAE_Linux - Flake with build and dev shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }: flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs {
        inherit system;
      };

      # Required build tools
      deps = with pkgs; [
        cmake
        gcc
      ];
    in
    {
      packages.default = pkgs.stdenv.mkDerivation {
        pname = "FAE_Linux";
        version = "0.1";

        src = ./.;

        nativeBuildInputs = with pkgs; [ cmake gcc ];

        buildInputs = deps;

        buildPhase = ''
          cmake .
          make -j8
        '';
        
        installPhase = ''
          mkdir -p $out/bin/
          cp ./out/bin/FAE_Linux $out/bin/FAE_Linux
        '';
      };

      devShells.default = pkgs.mkShell {
        buildInputs = deps;
        shellHook = ''
          echo "You're now in the FAE_Linux dev shell."
          echo "To build manually:"
          echo "  cmake . && make"
        '';
      };
    });
}
