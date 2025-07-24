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
          echo "Available commands: build, build_standalone, server"
          echo "1. Run 'build' to compile and place files in ./www"
          echo "2. Run 'server' to start a web server for the ./www directory"
          echo "3. Run 'build_standalone' (after 'build') to create a single HTML file"
          echo "-------------------------------"

          # Main build function
          build() {
            echo "Creating output directory ./www..."
            mkdir -p www
            echo "Compiling C++ to WASM..."
            # Corrected emcc command: Replaced _detector_find_peak with _detector_find_multiple_peaks
            emcc detector.cpp -o www/detector.js -s EXPORTED_RUNTIME_METHODS='["cwrap", "ccall", "getValue", "HEAPU8"]' -s EXPORTED_FUNCTIONS='["_detector_new", "_detector_free", "_detector_find_multiple_peaks", "_find_strongest_peak", "_malloc", "_free"]'
            if [ $? -ne 0 ]; then
                echo "emcc compilation failed."
                return 1
            fi
            echo "Copying HTML file to www/index.html..."
            cp detector.html www/index.html
            echo "Build complete. Files are in ./www"
          }

          # Function to build the standalone HTML
          build_standalone() {
            if [ ! -f www/detector.js ] || [ ! -f www/detector.wasm ] || [ ! -f www/index.html ]; then
              echo "Error: Required files not found in ./www directory."
              echo "Please run the 'build' command first."
              return 1
            fi

            echo "Running Python script to create standalone HTML..."
            python3 <<'EOF'
import base64
import re

print("--> Reading www/detector.wasm and encoding to Base64...")
with open("www/detector.wasm", "rb") as f:
    wasm_base64 = base64.b64encode(f.read()).decode("utf-8")

print("--> Reading Emscripten loader (www/detector.js)...")
with open("www/detector.js", "r") as f:
    loader_js = f.read()

print("--> Reading application HTML (www/index.html)...")
with open("www/index.html", "r") as f:
    html_content = f.read()

# Extract the main script content (the part inside Module.onRuntimeInitialized)
match = re.search(r'createModule\(\{[\s\S]*?\}\)\.then\(Module => \{([\s\S]*?)\}\);', html_content, re.DOTALL)
if not match:
    print('\nError: Could not find the main script block in the HTML file.')
    exit(1)
app_js = match.group(1)

# Clean up the original HTML by removing the script tag we are replacing.
final_html = re.sub(r'<script type="module">[\s\S]*?</script>', "", html_content)


# Construct the new, self-contained script block
replacement_block = f"""<script>
// --- Injected Emscripten Loader ---
{loader_js}

// --- Main Application Logic (Self-Contained) ---
// Define the Module object for the loader script to find
var Module = {{
    wasmBinary: (function() {{
        const wasmBase64 = "{wasm_base64}";
        const binary_string = window.atob(wasmBase64);
        const len = binary_string.length;
        const bytes = new Uint8Array(len);
        for (let i = 0; i < len; i++) {{
            bytes[i] = binary_string.charCodeAt(i);
        }}
        return bytes;
    }})(),
    onRuntimeInitialized: function() {{
        // The main app logic is now inside the .then() block
    }}
}};

// The original .then() block from the module script
createModule(Module).then(Module => {{{app_js}}});
</script>"""

# Add the new script block to the end of the body
final_html = final_html.replace('</body>', replacement_block + '\n</body>')

with open("www/detector_standalone.html", "w") as f:
    f.write(final_html)

print("\nSuccess! Standalone file created: www/detector_standalone.html")
EOF
          }

          # Server function
          server() {
            if [ ! -f www/index.html ]; then
                echo "Error: www/index.html not found. Please run 'build' first."
                return 1
            fi
            echo "Starting server for ./www directory..."
            echo "Access the standard version at: http://localhost:8000"
            if [ -f www/detector_standalone.html ]; then
                echo "Access the standalone version at: http://localhost:8000/detector_standalone.html"
            fi
            python3 -m http.server -d www 8000
          }
        '';
      };
    };
}

