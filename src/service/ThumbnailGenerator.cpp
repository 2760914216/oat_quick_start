#include "ThumbnailGenerator.hpp"
#include "ImageLoader.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

std::pair<int, int> ThumbnailGenerator::getSizeDimensions(const std::string& sizeType)
{
    if (sizeType == "list")
    {
        return {150, 150};
    }
    if (sizeType == "detail")
    {
        return {400, 400};
    }
    return {0, 0};
}

std::pair<int, int> ThumbnailGenerator::calculateAspectFit(int srcW, int srcH, int maxW, int maxH)
{
    if (srcW == 0 || srcH == 0)
    {
        return {0, 0};
    }

    float scaleX = static_cast<float>(maxW) / static_cast<float>(srcW);
    float scaleY = static_cast<float>(maxH) / static_cast<float>(srcH);
    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    int newW = static_cast<int>(static_cast<float>(srcW) * scale);
    int newH = static_cast<int>(static_cast<float>(srcH) * scale);

    return {newW, newH};
}

static unsigned char bilinearSample(const std::vector<unsigned char>& data, int width, int height, float x, float y)
{
    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    x0 = (x0 < 0) ? 0 : ((x0 >= width) ? width - 1 : x0);
    x1 = (x1 < 0) ? 0 : ((x1 >= width) ? width - 1 : x1);
    y0 = (y0 < 0) ? 0 : ((y0 >= height) ? height - 1 : y0);
    y1 = (y1 < 0) ? 0 : ((y1 >= height) ? height - 1 : y1);

    float fx = x - static_cast<float>(x0);
    float fy = y - static_cast<float>(y0);

    int idx00 = (y0 * width + x0) * 3;
    int idx01 = (y0 * width + x1) * 3;
    int idx10 = (y1 * width + x0) * 3;
    int idx11 = (y1 * width + x1) * 3;

    float r = (1 - fx) * (1 - fy) * static_cast<float>(data[idx00]) + fx * (1 - fy) * static_cast<float>(data[idx01]) +
              (1 - fx) * fy * static_cast<float>(data[idx10]) + fx * fy * static_cast<float>(data[idx11]);

    return static_cast<unsigned char>(r);
}

static std::optional<std::vector<unsigned char>> resizeImage(const std::vector<unsigned char>& srcData, int srcW, int srcH, int dstW, int dstH)
{
    if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
    {
        return std::nullopt;
    }

    std::vector<unsigned char> dstData(dstW * dstH * 3);

    float scaleX = static_cast<float>(srcW - 1) / static_cast<float>(dstW);
    float scaleY = static_cast<float>(srcH - 1) / static_cast<float>(dstH);

    for (int y = 0; y < dstH; ++y)
    {
        for (int x = 0; x < dstW; ++x)
        {
            float srcX = x * scaleX;
            float srcY = y * scaleY;

            int dstIdx = (y * dstW + x) * 3;
            dstData[dstIdx] = bilinearSample(srcData, srcW, srcH, srcX, srcY);
            dstData[dstIdx + 1] = bilinearSample(srcData, srcW, srcH, srcX + 0.5f, srcY);
            dstData[dstIdx + 2] = bilinearSample(srcData, srcW, srcH, srcX, srcY + 0.5f);
        }
    }

    return dstData;
}

std::optional<std::string> ThumbnailGenerator::saveThumbnail(const std::string& photoId, const std::string& sizeType, const std::vector<unsigned char>& data,
                                                             int width, int height)
{
    fs::path thumbDir = fs::path("res") / "Thumbnail";
    if (!fs::exists(thumbDir))
    {
        if (!fs::create_directories(thumbDir))
        {
            return std::nullopt;
        }
    }

    std::string filename = photoId + "_" + sizeType + ".jpg";
    fs::path thumbPath = thumbDir / filename;

    if (!ImageLoader::save(thumbPath.string(), data, width, height, 3))
    {
        return std::nullopt;
    }

    return thumbPath.string();
}

std::optional<std::string> ThumbnailGenerator::generateThumbnailWithDims(const std::string& photoId, const std::vector<unsigned char>& imageData,
                                                                         int originalWidth, int originalHeight, const std::string& sizeType)
{
    auto [targetW, targetH] = getSizeDimensions(sizeType);
    if (targetW == 0 || targetH == 0)
    {
        return std::nullopt;
    }

    auto [newW, newH] = calculateAspectFit(originalWidth, originalHeight, targetW, targetH);
    if (newW == 0 || newH == 0)
    {
        return std::nullopt;
    }

    auto resized = resizeImage(imageData, originalWidth, originalHeight, newW, newH);
    if (!resized)
    {
        return std::nullopt;
    }

    auto savedPath = saveThumbnail(photoId, sizeType, *resized, newW, newH);
    if (!savedPath)
    {
        return std::nullopt;
    }

    return savedPath;
}

std::optional<std::string> ThumbnailGenerator::generateThumbnail(const std::string& photoId, const std::vector<unsigned char>& imageData,
                                                                 const std::string& sizeType)
{
    int estimatedSize = 1024;
    int estimatedSide = static_cast<int>(std::sqrt(estimatedSize / 3));

    return generateThumbnailWithDims(photoId, imageData, estimatedSide, estimatedSide, sizeType);
}