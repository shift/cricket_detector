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
        packages = with pkgs; [ emscripten python3 ];
        shellHook = ''
          echo "--- WASM Detector Dev Shell ---"
          echo "Available commands: build, server"
          echo "1. Run 'build' to compile and place files in ./www"
          echo "2. Run 'server' to start a web server for the ./www directory"
          echo "-------------------------------"

          build() {
            echo "Creating output directory ./www..."
            mkdir -p www
            echo "Compiling C++ to WASM..."
            # Corrected emcc command: Exports _find_all_peaks and does NOT export the old _find_strongest_peak
            emcc detector.cpp -o www/detector.js -s EXPORTED_RUNTIME_METHODS='["cwrap", "ccall", "getValue", "HEAPU8", "HEAPF32"]' -s EXPORTED_FUNCTIONS='["_detector_new", "_detector_free", "_detector_find_multiple_peaks", "_find_all_peaks", "_malloc", "_free"]'
            if [ $? -ne 0 ]; then
                echo "emcc compilation failed."
                return 1
            fi
            echo "Copying HTML file to www/index.html..."
            cp detector.html www/index.html
            echo "Build complete. Files are in ./www"
          }

          server() {
            if [ ! -f www/index.html ]; then
                echo "Error: www/index.html not found. Please run 'build' first."
                return 1
            fi
            echo "Starting server for ./www directory..."
            echo "Access the app at: http://localhost:8000"
            python3 -m http.server -d www 8000
          }
        '';
      };
    };
}

