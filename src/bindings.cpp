// spz/src/bindings.cpp
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include "load-spz.h"
#include <vector>
#include <cstdint>
#include <stdexcept>

// Add this debug function
EM_JS(void, console_log, (const char* str), {
    console.log(UTF8ToString(str));
});

namespace emscripten {
    // Register std::vector<uint8_t> with Embind
    EMSCRIPTEN_BINDINGS(vector_uint8_t_bindings) {
        console_log("Registering vector bindings");
        register_vector<uint8_t>("vector_uint8_t");
    }
}

namespace spz {

// Wrapper function for saveSpz
std::vector<uint8_t> saveSpzWrapper(const GaussianCloud &g) {
    std::vector<uint8_t> output;
    bool success = saveSpz(g, &output);
    if (!success) {
        throw std::runtime_error("saveSpz failed");
    }
    return output;
}

// Todo: Not sure why this is needed
GaussianCloud loadSpzWrapper(const std::vector<uint8_t> &data) {
    GaussianCloud cloud = loadSpz(data);
    return cloud;
}

GaussianCloud loadSpzFromBuffer(const uint8_t* data, size_t length) {
    // Create a view of the data without copying
    std::vector<uint8_t> view(data, data + length);
    return loadSpz(view);
}

} // namespace spz

using namespace emscripten;

EMSCRIPTEN_BINDINGS(spz_bindings) {
    console_log("Registering spz bindings");
    
    // Bind GaussianCloud
    class_<spz::GaussianCloud>("GaussianCloud")
        .constructor<>()
        .property("numPoints", &spz::GaussianCloud::numPoints)
        .property("shDegree", &spz::GaussianCloud::shDegree)
        .property("antialiased", &spz::GaussianCloud::antialiased)
        .property("positions", &spz::GaussianCloud::positions)
        .property("scales", &spz::GaussianCloud::scales)
        .property("rotations", &spz::GaussianCloud::rotations)
        .property("alphas", &spz::GaussianCloud::alphas)
        .property("colors", &spz::GaussianCloud::colors)
        .property("sh", &spz::GaussianCloud::sh)
        ;

    function("loadSpz", &spz::loadSpzWrapper);
    function("loadSpzFromBuffer", &spz::loadSpzFromBuffer, allow_raw_pointers());
    function("saveSpz", &spz::saveSpzWrapper);
    
    // Retain allow_raw_pointers() only for functions that require it
    function("loadSplatFromPly", &spz::loadSplatFromPly, allow_raw_pointers());
    function("saveSplatToPly", &spz::saveSplatToPly, allow_raw_pointers());
    
    console_log("Finished registering bindings");
}