#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ImageLoader.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
#include <vector>

std::optional<std::vector<unsigned char>> ImageLoader::load(const std::string& filepath)
{
    int width, height, channels;
    stbi_uc* data = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb);
    if (!data)
    {
        return std::nullopt;
    }
    std::vector<unsigned char> imageData(data, data + width * height * 3);
    stbi_image_free(data);
    return imageData;
}

bool ImageLoader::save(const std::string& filepath, const std::vector<unsigned char>& data, int width, int height, int channels)
{
    int stride = width * channels;
    int result = 0;
    std::string ext = filepath.substr(filepath.find_last_of('.') + 1);
    if (ext == "png")
    {
        result = stbi_write_png(filepath.c_str(), width, height, channels, data.data(), stride);
    }
    else if (ext == "bmp")
    {
        result = stbi_write_bmp(filepath.c_str(), width, height, channels, data.data());
    }
    else if (ext == "tga")
    {
        result = stbi_write_tga(filepath.c_str(), width, height, channels, data.data());
    }
    else if (ext == "jpg" || ext == "jpeg")
    {
        result = stbi_write_jpg(filepath.c_str(), width, height, channels, data.data(), 90);
    }
    return result != 0;
}

int ImageLoader::get_channels(const std::string& filepath)
{
    int width, height, channels;
    if (!stbi_load(filepath.c_str(), &width, &height, &channels, 0))
    {
        return 0;
    }
    return channels;
}