#include "load-spz.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// Utility functions
int dimForDegree(int degree) {
  switch (degree) {
    case 0: return 0;
    case 1: return 3;
    case 2: return 8;
    case 3: return 15;
    default: return 0;
  }
}

float sigmoid(float x) {
  return 1.0f / (1.0f + std::exp(-x));
}

bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

std::string getFileExtension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    std::string ext = filename.substr(dot_pos);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext;
}

spz::GaussianCloud pruneGaussians(const spz::GaussianCloud& cloud, uint8_t alpha_threshold) {
    if (alpha_threshold == 0) {
        return cloud; // No pruning needed
    }
    
    spz::GaussianCloud pruned;
    pruned.shDegree = cloud.shDegree;
    pruned.antialiased = cloud.antialiased;
    
    // Process each gaussian and only keep those above the threshold
    const int shDim = dimForDegree(cloud.shDegree);
    
    for (int i = 0; i < cloud.numPoints; i++) {
        // Convert alpha to 0-255 range to compare with threshold
        float alpha = cloud.alphas[i];
        uint8_t alpha_byte = static_cast<uint8_t>(sigmoid(alpha) * 255.0f);
        
        if (alpha_byte >= alpha_threshold) {
            // Add this point to the pruned cloud
            for (int j = 0; j < 3; j++) {
                pruned.positions.push_back(cloud.positions[i * 3 + j]);
                pruned.scales.push_back(cloud.scales[i * 3 + j]);
                pruned.colors.push_back(cloud.colors[i * 3 + j]);
            }
            
            for (int j = 0; j < 4; j++) {
                pruned.rotations.push_back(cloud.rotations[i * 4 + j]);
            }
            
            pruned.alphas.push_back(alpha);
            
            for (int j = 0; j < shDim * 3; j++) {
                pruned.sh.push_back(cloud.sh[i * shDim * 3 + j]);
            }
        }
    }
    
    pruned.numPoints = pruned.alphas.size();
    
    if (alpha_threshold > 0) {
        std::cout << "Pruned " << (cloud.numPoints - pruned.numPoints) << " gaussians out of " 
                  << cloud.numPoints << " (alpha threshold = " << static_cast<int>(alpha_threshold) << ")\n";
    }
    
    return pruned;
}

void printUsage() {
    std::cerr << "Usage: spz_convert <input_file> <output_file> [options]\n";
    std::cerr << "Supported conversions:\n";
    std::cerr << "  .ply -> .spz\n";
    std::cerr << "  .spz -> .ply\n";
    std::cerr << "  .ply -> .ply (prune only)\n";
    std::cerr << "  .spz -> .spz (prune only)\n";
    std::cerr << "Options:\n";
    std::cerr << "  -a <threshold>  Prune gaussians with alpha below this value (0-255, default: 0)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage();
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];
    uint8_t alpha_threshold = 0;
    
    // Parse optional arguments
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-a") {
            if (i + 1 < argc) {
                try {
                    int threshold = std::stoi(argv[i + 1]);
                    if (threshold < 0 || threshold > 255) {
                        std::cerr << "Error: alpha threshold must be between 0 and 255\n";
                        return 1;
                    }
                    alpha_threshold = static_cast<uint8_t>(threshold);
                    i++; // Skip the next argument since we used it
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing alpha threshold: " << e.what() << "\n";
                    return 1;
                }
            } else {
                std::cerr << "Error: -a requires a value\n";
                return 1;
            }
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage();
            return 1;
        }
    }

    // Check if input file exists
    if (!fileExists(input_file)) {
        std::cerr << "Input file does not exist: " << input_file << "\n";
        return 1;
    }

    // Determine file extensions
    std::string input_extension = getFileExtension(input_file);
    std::string output_extension = getFileExtension(output_file);

    if (input_extension.empty()) {
        std::cerr << "Input file has no extension: " << input_file << "\n";
        return 1;
    }

    if (output_extension.empty()) {
        std::cerr << "Output file has no extension: " << output_file << "\n";
        return 1;
    }

    // Load the appropriate format
    spz::GaussianCloud cloud;
    if (input_extension == ".ply") {
        cloud = spz::loadSplatFromPly(input_file);
        if (cloud.numPoints == 0) {
            std::cerr << "Failed to load GaussianCloud from " << input_file << "\n";
            return 1;
        }
    } else if (input_extension == ".spz") {
        cloud = spz::loadSpz(input_file);
        if (cloud.numPoints == 0) {
            std::cerr << "Failed to load GaussianCloud from " << input_file << "\n";
            return 1;
        }
    } else {
        std::cerr << "Unsupported input file extension: " << input_extension << "\n";
        std::cerr << "Supported extensions are .ply and .spz\n";
        return 1;
    }

    // Apply pruning
    cloud = pruneGaussians(cloud, alpha_threshold);

    // Save in the appropriate format
    bool success = false;
    if (output_extension == ".spz") {
        success = spz::saveSpz(cloud, output_file);
    } else if (output_extension == ".ply") {
        success = spz::saveSplatToPly(cloud, output_file);
    } else {
        std::cerr << "Unsupported output file extension: " << output_extension << "\n";
        std::cerr << "Supported extensions are .ply and .spz\n";
        return 1;
    }

    if (!success) {
        std::cerr << "Failed to save GaussianCloud to " << output_file << "\n";
        return 1;
    }

    // Determine conversion type for output message
    std::string conversion_type;
    if (input_extension == output_extension) {
        conversion_type = "pruned";
    } else {
        conversion_type = "converted";
    }

    std::cout << "Successfully " << conversion_type << " " << input_file << " to " << output_file << "\n";

    return 0;
} 