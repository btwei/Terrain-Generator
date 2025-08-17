#ifndef GENERATOR_HPP
#define GENERATOR_HPP

#include <stdexcept>
#include <vector>

namespace tg {

struct Heightmap {
    std::vector<uint16_t> data;
    size_t width;
    size_t height;
};

void exportHeightmapAsR16(Heightmap& heightmap, const std::string& filepath);

void exportHeightmapAsObj(Heightmap& heightmap, const std::string& filepath);

} // namespace tg

#endif // GENERATOR_HPP