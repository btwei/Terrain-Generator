#include <iostream>
#include <stdexcept>
#include <string>

#include "tg/generator.hpp"

int main(int argc, char* argv[]) {
    std::string mode = "perlin";
    size_t size = 512;
    std::string path = "heightmap.r16";

    for(int i=1; i<argc; ++i) {
        std::string arg = argv[i];
        if(arg == "--mode" && i + 1 < argc) {
            mode = argv[++i];
        } else if(arg == "--size" && i + 1 < argc) {
            size = std::stoul(argv[++i]);
        } else if(arg == "--path" && i + 1 < argc) {
            path = argv[++i];
        } else if(arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --mode <mode>    Set the generation mode (default: perlin)\n"
                      << "  --size <size>    Set the size of the heightmap (default: 512)\n"
                      << "  --path <path>    Set the output path for the heightmap (default: heightmap)\n"
                      << "  --help          Show this help message\n"
                      << std::endl
                      << "Available modes: flat, random, perlin\n"
                      << "Note: The heightmap file type will depend on the specified file extension.\n"
                      << "Available extensions: .r16\n";
            return EXIT_SUCCESS;
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }

    tg::Heightmap heightmap;

    if(mode == "flat") {
        heightmap = tg::generateFlatHeightmap(size, size);
    } else if(mode == "random") {
        heightmap = tg::generateRandomHeightmap(size, size);
    } else if(mode == "perlin") {
        try {
            heightmap = tg::generatePerlinNoiseHeightmap(size, size);
        } catch(const std::runtime_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        throw std::invalid_argument("Unknown mode: " + mode);
    }

    try {
        tg::exportHeightmapAsR16(heightmap, path);
    } catch(const std::runtime_error& e) {
        std::cerr << "Error exporting heightmap: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}