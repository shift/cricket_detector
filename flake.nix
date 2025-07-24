{
  description = "A development environment for a WebAssembly audio detector";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux"; # Or your system, e.g., "aarch64-linux"
      pkgs = import nixpkgs { inherit system; };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        name = "wasm-detector-shell";

        # Packages available in the shell
        packages = with pkgs; [
          # The Emscripten SDK for compiling C/C++ to WebAssembly
          emscripten

          # A simple python web server to serve the files
          python3
        ];

        # Environment variables or shell hooks can be added here
        shellHook = ''
          echo "--- WASM Detector Dev Shell ---"
          echo "Available commands: emcc, python"
          echo "To compile: emcc detector.cpp -o detector.js -s MODULARIZE=1 -s EXPORTED_RUNTIME_METHODS='[\"cwrap\",\"ccall\"]' -s EXPORTED_FUNCTIONS='[\"_detector_new\",\"_detector_free\",\"_detector_find_peak\",\"_malloc\"]'"
          echo "To run server: python -m http.server"
          echo "-------------------------------"
        '';
      };
    };
}

