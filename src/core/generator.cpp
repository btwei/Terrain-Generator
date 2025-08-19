#include "tg/generator.hpp"

#include <fstream>
#include <random>

namespace tg {

Heightmap generateFlatHeightmap(size_t width, size_t height, uint16_t value) {
    Heightmap heights;
    heights.width = width;
    heights.height = height;

    heights.data.assign(width * height, value);

    return heights;
}

Heightmap generateRandomHeightmap(size_t width, size_t height) {
    Heightmap heights;
    heights.width = width;
    heights.height = height;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dis(0, 65535);
    for(size_t i = 0; i < width * height; i++) {
        heights.data.push_back(dis(gen));
    }

    return heights;
}

Heightmap generatePerlinNoiseHeightmap(size_t width, size_t height) {
    Heightmap heights;
    heights.width = width;
    heights.height = height;

    throw std::runtime_error("Perlin noise generation not implemented yet.");

    return heights;
}

void exportHeightmapAsR16(Heightmap& heightmap, const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if(!file) {
        throw std::runtime_error("Failed to open file for writing: " + filepath);
    }

    file.write(reinterpret_cast<const char*>(heightmap.data.data()), heightmap.data.size() * sizeof(uint16_t));
    file.close();

    fprintf(stdout, "Heightmap exported as R16 to %s\n", filepath.c_str());
}

void exportHeightmapAsObj(Heightmap& heightmap, const std::string& filepath) {
    throw std::runtime_error("OBJ export not implemented yet.");
}

} // namespace tg