#ifndef IMAGE_EDITOR_HPP
#define IMAGE_EDITOR_HPP

#include <optional>
#include <string>
#include <vector>

/**
 * @brief Edit parameters for image operations
 *
 * Stores all edit operations that can be applied to an image.
 * Operations are non-destructive - they return new EditParams
 * that can be stored and applied later.
 */
struct EditParams
{
    // Crop parameters
    int cropX = 0;
    int cropY = 0;
    int cropWidth = 0;  // 0 means no crop
    int cropHeight = 0; // 0 means no crop

    // Rotation: 0, 90, 180, 270 (clockwise degrees)
    int rotation = 0;

    // Resize dimensions: 0 means no resize
    int targetWidth = 0;
    int targetHeight = 0;

    // Original dimensions (for reference)
    int originalWidth = 0;
    int originalHeight = 0;

    // Brightness adjustment: -20 to +20
    int brightness = 0;

    // Contrast factor: 0.5 to 2.0
    float contrast = 1.0f;

    // Filter effects
    enum class Filter
    {
        NONE,
        GRAYSCALE,
        SEPIA,
        VINTAGE
    };
    Filter filter = Filter::NONE;

    /**
     * @brief Check if any edits have been applied
     */
    bool hasEdits() const
    {
        return cropWidth > 0 || cropHeight > 0 || rotation != 0 || targetWidth > 0 || targetHeight > 0 || brightness != 0 || contrast != 1.0f ||
               filter != Filter::NONE;
    }
};

/**
 * @brief ImageEditor service for crop/rotate/resize operations
 *
 * Provides non-destructive image editing operations.
 * All operations return new EditParams that can be stored
 * and applied to image data later.
 */
class ImageEditor
{
public:
    /**
     * @brief Apply crop operation
     *
     * @param params Current edit parameters
     * @param x Crop start X coordinate
     * @param y Crop start Y coordinate
     * @param width Crop width
     * @param height Crop height
     * @return New EditParams with crop applied
     */
    static EditParams crop(const EditParams& params, int x, int y, int width, int height);

    /**
     * @brief Apply 90-degree clockwise rotation
     *
     * @param params Current edit parameters
     * @return New EditParams with rotation applied
     */
    static EditParams rotate90cw(const EditParams& params);

    /**
     * @brief Apply 90-degree counter-clockwise rotation
     *
     * @param params Current edit parameters
     * @return New EditParams with rotation applied
     */
    static EditParams rotate90ccw(const EditParams& params);

    /**
     * @brief Apply 180-degree rotation
     *
     * @param params Current edit parameters
     * @return New EditParams with rotation applied
     */
    static EditParams rotate180(const EditParams& params);

    /**
     * @brief Apply resize operation
     *
     * @param params Current edit parameters
     * @param width Target width
     * @param height Target height
     * @return New EditParams with resize applied
     */
    static EditParams resize(const EditParams& params, int width, int height);

    /**
     * @brief Adjust brightness
     *
     * @param params Current edit parameters
     * @param delta Brightness delta (-20 to +20)
     * @return New EditParams with brightness applied
     */
    static EditParams adjustBrightness(const EditParams& params, int delta);

    /**
     * @brief Adjust contrast
     *
     * @param params Current edit parameters
     * @param factor Contrast factor (0.5 to 2.0)
     * @return New EditParams with contrast applied
     */
    static EditParams adjustContrast(const EditParams& params, float factor);

    /**
     * @brief Apply grayscale filter
     *
     * @param params Current edit parameters
     * @return New EditParams with grayscale filter applied
     */
    static EditParams applyGrayscale(const EditParams& params);

    /**
     * @brief Apply sepia filter
     *
     * @param params Current edit parameters
     * @return New EditParams with sepia filter applied
     */
    static EditParams applySepia(const EditParams& params);

    /**
     * @brief Apply vintage filter
     *
     * @param params Current edit parameters
     * @return New EditParams with vintage filter applied
     */
    static EditParams applyVintage(const EditParams& params);

    /**
     * @brief Apply all edits to image data
     *
     * @param imageData Raw image data (RGB)
     * @param params Edit parameters to apply
     * @return Edited image data on success, std::nullopt on failure
     */
    static std::optional<std::vector<unsigned char>> applyEdits(const std::vector<unsigned char>& imageData, const EditParams& params);

    /**
     * @brief Convert EditParams to JSON string for storage
     *
     * @param params Edit parameters
     * @return JSON string representation
     */
    static std::string toJson(const EditParams& params);

    /**
     * @brief Parse JSON string to EditParams
     *
     * @param json JSON string
     * @return EditParams on success, std::nullopt on failure
     */
    static std::optional<EditParams> fromJson(const std::string& json);

private:
    ImageEditor() = delete;
    ~ImageEditor() = delete;

    /**
     * @brief Apply crop to image data
     */
    static std::optional<std::vector<unsigned char>> applyCrop(const std::vector<unsigned char>& imageData, int srcW, int srcH, int x, int y, int width,
                                                               int height);

    /**
     * @brief Apply rotation to image data
     */
    static std::optional<std::vector<unsigned char>> applyRotation(const std::vector<unsigned char>& imageData, int width, int height, int rotation);

    /**
     * @brief Apply resize to image data
     */
    static std::optional<std::vector<unsigned char>> applyResize(const std::vector<unsigned char>& imageData, int srcW, int srcH, int dstW, int dstH);

    static std::vector<unsigned char> applyBrightnessAdjust(const std::vector<unsigned char>& imageData, int width, int height, int delta);

    static std::vector<unsigned char> applyContrastAdjust(const std::vector<unsigned char>& imageData, int width, int height, float factor);

    static std::vector<unsigned char> applyGrayscaleFilter(const std::vector<unsigned char>& imageData, int width, int height);

    static std::vector<unsigned char> applySepiaFilter(const std::vector<unsigned char>& imageData, int width, int height);

    static std::vector<unsigned char> applyVintageFilter(const std::vector<unsigned char>& imageData, int width, int height);
};

#endif // IMAGE_EDITOR_HPP