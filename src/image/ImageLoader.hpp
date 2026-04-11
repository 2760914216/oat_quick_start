#ifndef IMAGE_LOADER_HPP
#define IMAGE_LOADER_HPP

#include <optional>
#include <string>
#include <vector>

/**
 * @brief Wrapper class for stb_image library
 *
 * Provides static methods for loading and saving images using stb_image.
 * This wrapper handles image format detection, loading, and writing.
 */
class ImageLoader
{
public:
    /**
     * @brief Load an image from file
     *
     * @param filepath Path to the image file
     * @return std::optional<std::vector<unsigned char>> Image data as RGB/RGBA bytes, or std::nullopt on failure
     */
    static std::optional<std::vector<unsigned char>> load(const std::string& filepath);

    /**
     * @brief Save an image to file
     *
     * @param filepath Path to save the image (extension determines format: png, bmp, tga, jpg)
     * @param data Image data (RGB or RGBA)
     * @param width Image width in pixels
     * @param height Image height in pixels
     * @param channels Number of channels (3 for RGB, 4 for RGBA)
     * @return true on success, false on failure
     */
    static bool save(const std::string& filepath, const std::vector<unsigned char>& data, int width, int height, int channels);

    /**
     * @brief Get the number of channels in an image file
     *
     * @param filepath Path to the image file
     * @return Number of channels (1, 3, or 4), or 0 on failure
     */
    static int get_channels(const std::string& filepath);

private:
    // Prevent instantiation
    ImageLoader() = delete;
    ~ImageLoader() = delete;
};

#endif // IMAGE_LOADER_HPP