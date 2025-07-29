{
  description = "FAE_Linux - Flake with build and dev shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }: flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = nixpkgs.legacyPackages.${system};

      # Required build tools
    in
    {
      packages.default = pkgs.stdenv.mkDerivation {
        pname = "FAE_Linux";
        version = "0.1";

        src = ./.;

        nativeBuildInputs = with pkgs; [ cmake gcc ];


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
        inputsFrom = [ self.packages.${pkgs.system}.default ];
        shellHook = ''
          echo "You're now in the FAE_Linux dev shell."
          echo "To build manually:"
          echo "  cmake . && make"
        '';
      };
    });
}
