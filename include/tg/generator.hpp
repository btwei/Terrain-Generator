#ifndef GENERATOR_HPP
#define GENERATOR_HPP

#include <stdexcept>
#include <vector>

namespace tg {

struct Heightmap {
    std::vector<uint16_t> data; //row-major order
    size_t width;
    size_t height;
};

struct Attributes {
    float x,y,z;
    float n_x, n_y, n_z;
    float u, v;
};

struct Mesh {
    std::vector<Attributes> interleavedAttributes;
    std::vector<uint32_t> indices;
};

Heightmap generateFlatHeightmap(size_t width, size_t height, uint16_t value = 32768);

Heightmap generateRandomHeightmap(size_t width, size_t height);

Heightmap generatePerlinNoiseHeightmap(size_t width, size_t height, size_t gridResolution);

Heightmap generateDiamondSquareHeightmap(size_t width, size_t height, float roughness);

Mesh convertHeightmapToMesh(const Heightmap& heightmap);

void exportHeightmapAsR16(Heightmap& heightmap, const std::string& filepath);

void exportHeightmapAsObj(Heightmap& heightmap, const std::string& filepath);

} // namespace tg

#endif // GENERATOR_HPP