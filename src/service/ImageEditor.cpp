#include "ImageEditor.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

EditParams ImageEditor::crop(const EditParams& params, int x, int y, int width, int height)
{
    EditParams result = params;
    result.cropX = x;
    result.cropY = y;
    result.cropWidth = width;
    result.cropHeight = height;
    return result;
}

EditParams ImageEditor::rotate90cw(const EditParams& params)
{
    EditParams result = params;
    result.rotation = (result.rotation + 90) % 360;
    return result;
}

EditParams ImageEditor::rotate90ccw(const EditParams& params)
{
    EditParams result = params;
    result.rotation = (result.rotation + 270) % 360;
    return result;
}

EditParams ImageEditor::rotate180(const EditParams& params)
{
    EditParams result = params;
    result.rotation = (result.rotation + 180) % 360;
    return result;
}

EditParams ImageEditor::resize(const EditParams& params, int width, int height)
{
    EditParams result = params;
    result.targetWidth = width;
    result.targetHeight = height;
    return result;
}

EditParams ImageEditor::adjustBrightness(const EditParams& params, int delta)
{
    EditParams result = params;
    result.brightness = std::max(-20, std::min(20, delta));
    return result;
}

EditParams ImageEditor::adjustContrast(const EditParams& params, float factor)
{
    EditParams result = params;
    result.contrast = std::max(0.5f, std::min(2.0f, factor));
    return result;
}

EditParams ImageEditor::applyGrayscale(const EditParams& params)
{
    EditParams result = params;
    result.filter = EditParams::Filter::GRAYSCALE;
    return result;
}

EditParams ImageEditor::applySepia(const EditParams& params)
{
    EditParams result = params;
    result.filter = EditParams::Filter::SEPIA;
    return result;
}

EditParams ImageEditor::applyVintage(const EditParams& params)
{
    EditParams result = params;
    result.filter = EditParams::Filter::VINTAGE;
    return result;
}

std::optional<std::vector<unsigned char>> ImageEditor::applyCrop(const std::vector<unsigned char>& imageData, int srcW, int srcH, int x, int y, int width,
                                                                 int height)
{
    if (x < 0 || y < 0 || x + width > srcW || y + height > srcH || width <= 0 || height <= 0)
    {
        return std::nullopt;
    }

    std::vector<unsigned char> result(width * height * 3);
    for (int row = 0; row < height; ++row)
    {
        for (int col = 0; col < width; ++col)
        {
            int srcIdx = ((y + row) * srcW + (x + col)) * 3;
            int dstIdx = (row * width + col) * 3;
            result[dstIdx] = imageData[srcIdx];
            result[dstIdx + 1] = imageData[srcIdx + 1];
            result[dstIdx + 2] = imageData[srcIdx + 2];
        }
    }

    return result;
}

std::optional<std::vector<unsigned char>> ImageEditor::applyRotation(const std::vector<unsigned char>& imageData, int width, int height, int rotation)
{
    if (rotation == 0)
    {
        return imageData;
    }

    int newW = width;
    int newH = height;

    if (rotation == 90 || rotation == 270)
    {
        newW = height;
        newH = width;
    }

    std::vector<unsigned char> result(newW * newH * 3);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int srcIdx = (y * width + x) * 3;
            int dstX, dstY;

            if (rotation == 90)
            {
                dstX = height - 1 - y;
                dstY = x;
            }
            else if (rotation == 180)
            {
                dstX = width - 1 - x;
                dstY = height - 1 - y;
            }
            else if (rotation == 270)
            {
                dstX = y;
                dstY = width - 1 - x;
            }
            else
            {
                return std::nullopt;
            }

            int dstIdx = (dstY * newW + dstX) * 3;
            result[dstIdx] = imageData[srcIdx];
            result[dstIdx + 1] = imageData[srcIdx + 1];
            result[dstIdx + 2] = imageData[srcIdx + 2];
        }
    }

    return result;
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

std::optional<std::vector<unsigned char>> ImageEditor::applyResize(const std::vector<unsigned char>& imageData, int srcW, int srcH, int dstW, int dstH)
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
            dstData[dstIdx] = bilinearSample(imageData, srcW, srcH, srcX, srcY);
            dstData[dstIdx + 1] = bilinearSample(imageData, srcW, srcH, srcX + 0.5f, srcY);
            dstData[dstIdx + 2] = bilinearSample(imageData, srcW, srcH, srcX, srcY + 0.5f);
        }
    }

    return dstData;
}

std::vector<unsigned char> ImageEditor::applyBrightnessAdjust(const std::vector<unsigned char>& imageData, int width, int height, int delta)
{
    std::vector<unsigned char> result = imageData;
    for (size_t i = 0; i < result.size(); ++i)
    {
        int val = static_cast<int>(result[i]) + delta;
        result[i] = static_cast<unsigned char>(std::max(0, std::min(255, val)));
    }
    return result;
}

std::vector<unsigned char> ImageEditor::applyContrastAdjust(const std::vector<unsigned char>& imageData, int width, int height, float factor)
{
    const float midpoint = 128.0f;
    std::vector<unsigned char> result = imageData;
    for (size_t i = 0; i < result.size(); ++i)
    {
        float val = static_cast<float>(result[i]);
        val = midpoint + (val - midpoint) * factor;
        result[i] = static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, val)));
    }
    return result;
}

std::vector<unsigned char> ImageEditor::applyGrayscaleFilter(const std::vector<unsigned char>& imageData, int width, int height)
{
    std::vector<unsigned char> result(imageData.size());
    for (int i = 0; i < width * height; ++i)
    {
        int idx = i * 3;
        float gray =
            0.299f * static_cast<float>(imageData[idx]) + 0.587f * static_cast<float>(imageData[idx + 1]) + 0.114f * static_cast<float>(imageData[idx + 2]);
        unsigned char g = static_cast<unsigned char>(gray);
        result[idx] = g;
        result[idx + 1] = g;
        result[idx + 2] = g;
    }
    return result;
}

std::vector<unsigned char> ImageEditor::applySepiaFilter(const std::vector<unsigned char>& imageData, int width, int height)
{
    std::vector<unsigned char> result = imageData;
    for (int i = 0; i < width * height; ++i)
    {
        int idx = i * 3;
        int r = static_cast<int>(imageData[idx]);
        int g = static_cast<int>(imageData[idx + 1]);
        int b = static_cast<int>(imageData[idx + 2]);

        int tr = static_cast<int>(0.393f * r + 0.769f * g + 0.189f * b);
        int tg = static_cast<int>(0.349f * r + 0.686f * g + 0.168f * b);
        int tb = static_cast<int>(0.272f * r + 0.534f * g + 0.131f * b);

        result[idx] = static_cast<unsigned char>(std::min(255, tr));
        result[idx + 1] = static_cast<unsigned char>(std::min(255, tg));
        result[idx + 2] = static_cast<unsigned char>(std::min(255, tb));
    }
    return result;
}

std::vector<unsigned char> ImageEditor::applyVintageFilter(const std::vector<unsigned char>& imageData, int width, int height)
{
    std::vector<unsigned char> result = applySepiaFilter(imageData, width, height);
    float cx = static_cast<float>(width) / 2.0f;
    float cy = static_cast<float>(height) / 2.0f;
    float maxDist = std::sqrt(cx * cx + cy * cy);
    for (int i = 0; i < width * height; ++i)
    {
        int idx = i * 3;
        int x = i % width;
        int y = i / width;
        float dist = std::sqrt(static_cast<float>(x * x + y * y));
        float vignette = 1.0f - (dist / maxDist) * 0.5f;
        vignette = std::max(0.4f, vignette);
        for (int c = 0; c < 3; ++c)
        {
            float val = static_cast<float>(result[idx + c]) * vignette;
            result[idx + c] = static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, val)));
        }
    }
    return result;
}

std::optional<std::vector<unsigned char>> ImageEditor::applyEdits(const std::vector<unsigned char>& imageData, const EditParams& params)
{
    if (!params.hasEdits())
    {
        return imageData;
    }

    int currentW = params.originalWidth;
    int currentH = params.originalHeight;
    std::vector<unsigned char> currentData = imageData;

    if (params.cropWidth > 0 && params.cropHeight > 0)
    {
        auto cropped = applyCrop(currentData, currentW, currentH, params.cropX, params.cropY, params.cropWidth, params.cropHeight);
        if (!cropped)
        {
            return std::nullopt;
        }
        currentData = std::move(*cropped);
        currentW = params.cropWidth;
        currentH = params.cropHeight;
    }

    if (params.rotation != 0)
    {
        auto rotated = applyRotation(currentData, currentW, currentH, params.rotation);
        if (!rotated)
        {
            return std::nullopt;
        }
        currentData = std::move(*rotated);
        if (params.rotation == 90 || params.rotation == 270)
        {
            std::swap(currentW, currentH);
        }
    }

    if (params.targetWidth > 0 && params.targetHeight > 0)
    {
        auto resized = applyResize(currentData, currentW, currentH, params.targetWidth, params.targetHeight);
        if (!resized)
        {
            return std::nullopt;
        }
        currentData = std::move(*resized);
    }

    if (params.brightness != 0)
    {
        currentData = applyBrightnessAdjust(currentData, currentW, currentH, params.brightness);
    }

    if (params.contrast != 1.0f)
    {
        currentData = applyContrastAdjust(currentData, currentW, currentH, params.contrast);
    }

    if (params.filter == EditParams::Filter::GRAYSCALE)
    {
        currentData = applyGrayscaleFilter(currentData, currentW, currentH);
    }
    else if (params.filter == EditParams::Filter::SEPIA)
    {
        currentData = applySepiaFilter(currentData, currentW, currentH);
    }
    else if (params.filter == EditParams::Filter::VINTAGE)
    {
        currentData = applyVintageFilter(currentData, currentW, currentH);
    }

    return currentData;
}

std::string ImageEditor::toJson(const EditParams& params)
{
    std::ostringstream oss;
    oss << "{";
    oss << "\"cropX\":" << params.cropX << ",";
    oss << "\"cropY\":" << params.cropY << ",";
    oss << "\"cropWidth\":" << params.cropWidth << ",";
    oss << "\"cropHeight\":" << params.cropHeight << ",";
    oss << "\"rotation\":" << params.rotation << ",";
    oss << "\"targetWidth\":" << params.targetWidth << ",";
    oss << "\"targetHeight\":" << params.targetHeight << ",";
    oss << "\"originalWidth\":" << params.originalWidth << ",";
    oss << "\"originalHeight\":" << params.originalHeight << ",";
    oss << "\"brightness\":" << params.brightness << ",";
    oss << "\"contrast\":" << params.contrast << ",";
    oss << "\"filter\":" << static_cast<int>(params.filter);
    oss << "}";
    return oss.str();
}

std::optional<EditParams> ImageEditor::fromJson(const std::string& json)
{
    EditParams params;
    std::istringstream iss(json);
    char ch;

    iss >> ch;
    if (ch != '{')
    {
        return std::nullopt;
    }

    while (iss >> ch && ch != '}')
    {
        if (ch == '"')
        {
            std::string key;
            while (iss >> ch && ch != '"')
            {
                key += ch;
            }
            iss >> ch;

            if (key == "filter")
            {
                int filterVal = 0;
                iss >> filterVal;
                params.filter = static_cast<EditParams::Filter>(filterVal);
            }
            else if (key == "contrast")
            {
                float fval = 0.0f;
                iss >> fval;
                params.contrast = fval;
            }
            else
            {
                int value = 0;
                iss >> value;
                iss >> ch;

                if (key == "cropX")
                    params.cropX = value;
                else if (key == "cropY")
                    params.cropY = value;
                else if (key == "cropWidth")
                    params.cropWidth = value;
                else if (key == "cropHeight")
                    params.cropHeight = value;
                else if (key == "rotation")
                    params.rotation = value;
                else if (key == "targetWidth")
                    params.targetWidth = value;
                else if (key == "targetHeight")
                    params.targetHeight = value;
                else if (key == "originalWidth")
                    params.originalWidth = value;
                else if (key == "originalHeight")
                    params.originalHeight = value;
                else if (key == "brightness")
                    params.brightness = value;
            }
        }
    }

    return params;
}