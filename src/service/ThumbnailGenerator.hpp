#ifndef THUMBNAIL_GENERATOR_HPP
#define THUMBNAIL_GENERATOR_HPP

#include <optional>
#include <string>
#include <vector>

/**
 * @brief Thumbnail size types
 */
enum class ThumbnailSize
{
    LIST = 150,  // 150x150 for list view
    DETAIL = 400 // 400x400 for detail view
};

/**
 * @brief ThumbnailGenerator service for creating image thumbnails
 *
 * Provides methods to generate various sized thumbnails from photo data.
 * Uses stb_image_resize for high-quality scaling.
 */
class ThumbnailGenerator
{
public:
    /**
     * @brief Generate a thumbnail from image data
     *
     * @param photoId Unique identifier for the photo
     * @param imageData Raw image data (RGB)
     * @param sizeType Size type: "list" for 150x150, "detail" for 400x400
     * @return Thumbnail record ID on success, std::nullopt on failure
     */
    static std::optional<std::string> generateThumbnail(const std::string& photoId, const std::vector<unsigned char>& imageData, const std::string& sizeType);

    /**
     * @brief Get thumbnail dimensions for a size type
     *
     * @param sizeType Size type string
     * @return Pair of (width, height), or (0, 0) if invalid
     */
    static std::pair<int, int> getSizeDimensions(const std::string& sizeType);

    /**
     * @brief Generate a thumbnail from raw bytes with known dimensions
     *
     * @param photoId Unique identifier for the photo
     * @param imageData Raw image data (RGB)
     * @param originalWidth Original image width
     * @param originalHeight Original image height
     * @param sizeType Size type: "list" or "detail"
     * @return Thumbnail record ID on success, std::nullopt on failure
     */
    static std::optional<std::string> generateThumbnailWithDims(const std::string& photoId, const std::vector<unsigned char>& imageData, int originalWidth,
                                                                int originalHeight, const std::string& sizeType);

private:
    ThumbnailGenerator() = delete;
    ~ThumbnailGenerator() = delete;

    /**
     * @brief Calculate aspect-fit dimensions
     *
     * @param srcW Source width
     * @param srcH Source height
     * @param maxW Maximum target width
     * @param maxH Maximum target height
     * @return Pair of (targetWidth, targetHeight)
     */
    static std::pair<int, int> calculateAspectFit(int srcW, int srcH, int maxW, int maxH);

    /**
     * @brief Save thumbnail to file
     *
     * @param photoId Photo ID
     * @param sizeType Size type
     * @param data Image data
     * @param width Width
     * @param height Height
     * @return Saved file path on success, std::nullopt on failure
     */
    static std::optional<std::string> saveThumbnail(const std::string& photoId, const std::string& sizeType, const std::vector<unsigned char>& data, int width,
                                                    int height);
};

#endif // THUMBNAIL_GENERATOR_HPP