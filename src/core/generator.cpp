#include "tg/generator.hpp"

#include <iostream>
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

Mesh convertHeightmapToMesh(const Heightmap& heightmap) {
    Mesh mesh;

    size_t width = heightmap.width;
    size_t height = heightmap.height;

    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> uvs;

    // Generate Vertices
    for(size_t y=0; y < height; y++) {
        for(size_t x=0; x < width; x++) {
            positions.push_back(static_cast<float>(x) / width);
            positions.push_back(static_cast<float>(y) / height);
            positions.push_back(heightmap.data[y*height + x] / static_cast<float>(UINT16_MAX));

            /* Yes, the position x, y = u, v; it's redundant data, perhaps will remove later */
            uvs.push_back(static_cast<float>(x) / width);
            uvs.push_back(static_cast<float>(y) / height);
        }
    }

    // Generate Indices
    for(size_t y=0; y < height - 1; y++) {
        for(size_t x=0; x < width - 1; x++) {
            uint32_t topLeft = y*height + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (y+1)*height + x;
            uint32_t bottomRight = bottomLeft + 1;

            mesh.indices.push_back(topLeft);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(topRight);

            mesh.indices.push_back(topRight);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(bottomRight);
        }
    }

    // Compute Normals
    for(size_t y=0; y < height - 1; y++) {
        for(size_t x=0; x < width - 1; x++) {
            // Calculate dx and dy, minding heightmap boundary conditions
            float dx = (x == 0) ? heightmap.data[y*height+x+1] - heightmap.data[y*height+x]
                     : (x == width - 1) ? heightmap.data[y*height+x] - heightmap.data[y*height+x-1]
                     : (heightmap.data[y*height+x+1] - heightmap.data[y*height+x-1]) * 0.5;
            float dy = (y == 0) ? heightmap.data[(y+1)*height+x] - heightmap.data[y*height+x]
                     : (y == height - 1) ? heightmap.data[y*height+x] - heightmap.data[(y-1)*height+x]
                     : (heightmap.data[(y+1)*height+x] - heightmap.data[(y-1)*height+x]) * 0.5;

            float magnitude = dx*dx + dy*dy + 1;

            normals.push_back(dx/magnitude);
            normals.push_back(-1.0f/magnitude);
            normals.push_back(dy/magnitude);
        }
    }

    // Combine into interleaved array
    for(size_t i = 0; i < width*height; i++) {
        float x = positions[3*i];
        float y = positions[3*i + 1];
        float z = positions[3*i + 2];
        float n_x = normals[3*i];
        float n_y = normals[3*i + 1];
        float n_z = normals[3*i + 2];
        float u = uvs[2*i];
        float v = uvs[2*i + 1];

        mesh.interleavedAttributes.push_back({x, y, z, n_x, n_y, n_z, u, v});
    }

    return mesh;
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