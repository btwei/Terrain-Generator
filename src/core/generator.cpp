#include "tg/generator.hpp"

#include <iostream>
#include <fstream>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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

Heightmap generatePerlinNoiseHeightmap(size_t width, size_t height, size_t gridResolution) {
    Heightmap heights;
    heights.width = width;
    heights.height = height;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::vector<std::vector<glm::vec2>> vectorGrid(gridResolution + 1, std::vector<glm::vec2>(gridResolution + 1, glm::vec2()));
    for(size_t y = 0; y < gridResolution + 1; y++) {
        for(size_t x = 0; x < gridResolution + 1; x++) {
            std::uniform_real_distribution<float> dis(0.0f, glm::two_pi<float>());
            float angle = dis(gen);
            vectorGrid[x][y] = glm::vec2(std::cos(angle), std::sin(angle));
        }
    }

    float cellWidth = static_cast<float>(width) / gridResolution;
    float cellHeight = static_cast<float>(height) / gridResolution;

    for(size_t y = 0; y < height; y++) {
        for(size_t x = 0; x < width; x++) {
            size_t cellX = floor(x / cellWidth);
            size_t cellY = floor(y / cellHeight);
            float localX = (x / cellWidth) - cellX;
            float localY = (y / cellHeight) - cellY;

            float dotTL = glm::dot(vectorGrid[cellX][cellY], glm::vec2(localX, localY));
            float dotTR = glm::dot(vectorGrid[cellX+1][cellY], glm::vec2(localX-1, localY));
            float dotBL = glm::dot(vectorGrid[cellX][cellY+1], glm::vec2(localX, localY-1));
            float dotBR = glm::dot(vectorGrid[cellX+1][cellY+1], glm::vec2(localX-1, localY-1));
        
            float u = 6*pow(localX, 5) - 15*pow(localX, 4) + 10*pow(localX, 3);
            float v = 6*pow(localY, 5) - 15*pow(localY, 4) + 10*pow(localY, 3);
        
            float nx0 = glm::mix(dotTL, dotTR, u);
            float nx1 = glm::mix(dotBL, dotBR, u);
            float nxy = glm::mix(nx0, nx1, v);

            heights.data.push_back(static_cast<uint16_t>((nxy + 1.0f) / 2.0f * UINT16_MAX));
        }
    }

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
    for(size_t y=0; y < height; y++) {
        for(size_t x=0; x < width; x++) {
            // Calculate dx and dy, minding heightmap boundary conditions
            glm::vec3 RL = (x == 0) ? glm::vec3(1, 0, heightmap.data[y*height+x+1] - heightmap.data[y*height+x])
                     : (x == width) ? glm::vec3(1, 0, heightmap.data[y*height+x] - heightmap.data[y*height+x-1])
                     : glm::vec3(2.0f / width, 0.0f, positions[3*y*width+3*(x+1)+2] - positions[3*y*width+3*(x-1)+2]);
            glm::vec3 UD = (y == 0) ? glm::vec3(0, -1, heightmap.data[(y+1)*height+x] - heightmap.data[y*height+x])
                     : (y == height) ? glm::vec3(0, -1, heightmap.data[y*height+x] - heightmap.data[(y-1)*height+x])
                     : glm::vec3(0.0f, 2.0f / height, positions[3*(y+1)*width+3*x+2] - positions[3*(y-1)*width+3*x+2]);

            glm::vec3 normal = glm::normalize(glm::cross(RL, UD));

            normals.push_back(normal.x);
            normals.push_back(normal.y);
            normals.push_back(normal.z);
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