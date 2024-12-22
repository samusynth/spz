#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Enable debug mode to print each command before execution (optional)
#set -x

# Determine the directory where the script resides
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_ROOT="${SCRIPT_DIR}"
NATIVE_BUILD_DIR="${BUILD_ROOT}/build_native"
WASM_BUILD_DIR="${BUILD_ROOT}/build_wasm"

# Emscripten environment setup
# Make sure to update this path to your actual emsdk installation
EMSDK_DIR="${HOME}/Workspace/emsdk"
if [ ! -d "${EMSDK_DIR}" ]; then
  echo "Emscripten SDK not found at ${EMSDK_DIR}. Please update the EMSDK_DIR variable."
  exit 1
fi
source "${EMSDK_DIR}/emsdk_env.sh"

# Function to build native
build_native() {
  echo "=== Building Native Version ==="

  # Create build directory if it doesn't exist
  if [ ! -d "${NATIVE_BUILD_DIR}" ]; then
    mkdir -p "${NATIVE_BUILD_DIR}"
    echo "Created directory: ${NATIVE_BUILD_DIR}"
  fi

  cd "${NATIVE_BUILD_DIR}"

  echo "Configuring native build in $(pwd)..."
  
  # Configure the native build with verbose output for debugging
  cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=ON || {
    echo "CMake configuration for native build failed."
    exit 1
  }

  echo "Starting native build..."
  
  # Build using all available cores
  make -j"$(nproc)" || {
    echo "Make failed for native build."
    exit 1
  }

  echo "Native build completed successfully."

  cd "${BUILD_ROOT}"
}

# Function to build WASM
build_wasm() {
  echo "=== Building WASM Version ==="

  # Create build directory if it doesn't exist
  if [ ! -d "${WASM_BUILD_DIR}" ]; then
    mkdir -p "${WASM_BUILD_DIR}"
    echo "Created directory: ${WASM_BUILD_DIR}"
  fi

  cd "${WASM_BUILD_DIR}"

  echo "Configuring WASM build in $(pwd)..."
  
  # Configure the WASM build using Emscripten's toolchain with verbose output for debugging
  emcmake cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="${EMSDK_DIR}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" \
    -DCMAKE_VERBOSE_MAKEFILE=ON || {
      echo "CMake configuration for WASM build failed."
      exit 1
    }

  echo "Starting WASM build..."
  
  # Build using Emscripten
  emmake make spz_bindings || {
    echo "Make failed for WASM build."
    exit 1
  }

  echo "WASM build completed successfully."

  cd "${BUILD_ROOT}"
}

# Function to clean build directories (optional but recommended)
clean_builds() {
  echo "=== Cleaning Previous Builds ==="
  rm -rf "${NATIVE_BUILD_DIR}" "${WASM_BUILD_DIR}"
  echo "Cleaned build directories."
}

# Clean previous builds
clean_builds

# Build both targets
build_native
build_wasm

echo "=== All Builds Completed Successfully ==="