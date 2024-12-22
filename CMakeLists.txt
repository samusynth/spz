cmake_minimum_required(VERSION 3.10)

project(spz
  DESCRIPTION "A 3D Gaussians format"
  LANGUAGES C CXX
  VERSION 0.1)

include(GNUInstallDirs)

# Detect if compiling with Emscripten
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(IS_WASM TRUE)

  # Set Emscripten-specific compile and link flags
  set(USE_ZLIB_FLAGS "-s USE_ZLIB=1")
  set(EXTRA_FLAGS "-s ASSERTIONS=1 -s EXPORT_ES6=1 -s MODULARIZE=1 -s ENVIRONMENT=web -s NO_DISABLE_EXCEPTION_CATCHING=1 -s USE_ES6_IMPORT_META=0")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_ZLIB_FLAGS} ${EXTRA_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_ZLIB_FLAGS} ${EXTRA_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_ZLIB_FLAGS} ${EXTRA_FLAGS}")

else()
  set(IS_WASM FALSE)
endif()

# Set global output directories relative to the current build directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files for the spz library
set(spz_sources
  "${CMAKE_CURRENT_SOURCE_DIR}/src/cc/load-spz.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/cc/splat-c-types.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/cc/splat-types.cc"
)

# Header files for the spz library
set(spz_headers
  "${CMAKE_CURRENT_SOURCE_DIR}/src/cc/load-spz.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/cc/splat-c-types.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/cc/splat-types.h"
)

# Define the spz library
add_library(spz STATIC ${spz_sources})

# Conditional Handling of ZLIB
if(NOT IS_WASM)
  # Find ZLIB for native builds
  find_package(ZLIB REQUIRED)
  target_link_libraries(spz PRIVATE ZLIB::ZLIB)
endif()

# Specify include directories for the spz library
target_include_directories(spz
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/cc>
  INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Specify public headers for installation
set_target_properties(spz PROPERTIES
  PUBLIC_HEADER "${spz_headers}"
)

# Installation configurations
include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/spzConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/spzConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/spz"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/spzConfigVersion.cmake"
  VERSION "${spz_VERSION}"
  COMPATIBILITY SameMajorVersion
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/spzConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/spzConfigVersion.cmake"
  DESTINATION
    "${CMAKE_INSTALL_LIBDIR}/cmake/spz"
)

install(TARGETS spz
  EXPORT spzTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT spzTargets
  NAMESPACE spz::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/spz"
)

# Add executable for converting .ply to .spz (only for native builds)
if(NOT IS_WASM)
  add_executable(spz_convert src/main.cpp)
  
  # Link the spz library
  target_link_libraries(spz_convert PRIVATE spz)
  
  # Optionally, set the executable to install
  install(TARGETS spz_convert
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

# Add Embind bindings for WASM build
if(IS_WASM)
  # Create an executable target for WASM bindings
  add_executable(spz_bindings ${spz_sources} ${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp)

  target_include_directories(spz_bindings PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/cc)

  # Compiler-specific flags
  target_compile_options(spz_bindings PRIVATE
    "-O3"
  )

  # Updated linker flags to properly export the bindings
  target_link_options(spz_bindings PRIVATE
    "--bind"
    "-s ALLOW_MEMORY_GROWTH=1"
    "-s WASM=1"
    "-s MODULARIZE=1"
    "-s USE_ES6_IMPORT_META=0"
    "-s ENVIRONMENT=web"
    "-s NO_DISABLE_EXCEPTION_CATCHING"
    "-O3"
  )

  set(WASM_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build_wasm/wasm")
  
  # Modify existing targets to output to the static directory
  set_target_properties(spz_bindings PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${WASM_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${WASM_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${WASM_OUTPUT_DIR}"
  )
  
  # Optionally, copy the WASM build outputs to the Svelte static directory after build
  add_custom_command(TARGET spz_bindings POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${WASM_OUTPUT_DIR}/"
      "${CMAKE_CURRENT_SOURCE_DIR}/../vid2scene_server/viewer/static/src/spz_wasm/"
  )
endif()
