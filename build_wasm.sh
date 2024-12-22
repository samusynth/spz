#!/bin/bash

# Exit on error
set -e

# Source Emscripten environment (update path to your emsdk location)
source ~/Workspace/emsdk/emsdk_env.sh

# Create output directory
mkdir -p build_wasm/wasm

# Compile the code with more verbose output
em++ --verbose \
     src/cc/load-spz.cc \
     src/cc/splat-c-types.cc \
     src/cc/splat-types.cc \
     src/bindings.cpp \
     -I src/cc \
     -s WASM=1 \
     -s ALLOW_MEMORY_GROWTH=1 \
     -s MODULARIZE=1 \
     -s ENVIRONMENT=web \
     -s USE_ZLIB=1 \
     -s NO_DISABLE_EXCEPTION_CATCHING=1 \
     -s ASSERTIONS=1 \
     -s EXPORT_ES6=1 \
     -s EXPORTED_RUNTIME_METHODS=["FS"] \
     -lembind \
     -O3 \
     -o build_wasm/wasm/spz_bindings.js

# Copy to Svelte static directory (adjust path as needed)
cp build_wasm/wasm/* ../vid2scene_server/viewer/static/src/spz_wasm/