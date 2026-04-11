// ImageEditor tests - crop, rotate, resize, brightness, contrast, filter

#include "ImageEditor.hpp"
#include <gtest/gtest.h>

namespace image
{

class ImageEditorTest : public ::testing::Test
{
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(ImageEditorTest, CropBasic)
{
    EditParams params;
    auto result = ImageEditor::crop(params, 10, 20, 100, 80);
    EXPECT_EQ(result.cropX, 10);
    EXPECT_EQ(result.cropY, 20);
    EXPECT_EQ(result.cropWidth, 100);
    EXPECT_EQ(result.cropHeight, 80);
}

TEST_F(ImageEditorTest, CropNonDestructive)
{
    EditParams params;
    params.brightness = 5;
    auto result = ImageEditor::crop(params, 0, 0, 50, 50);
    EXPECT_EQ(result.brightness, 5);
    EXPECT_EQ(result.cropWidth, 50);
}

TEST_F(ImageEditorTest, Rotate90CW)
{
    EditParams params;
    auto result = ImageEditor::rotate90cw(params);
    EXPECT_EQ(result.rotation, 90);
}

TEST_F(ImageEditorTest, Rotate90CWChained)
{
    EditParams params;
    auto result = ImageEditor::rotate90cw(params);
    result = ImageEditor::rotate90cw(result);
    EXPECT_EQ(result.rotation, 180);
}

TEST_F(ImageEditorTest, Rotate90CCW)
{
    EditParams params;
    auto result = ImageEditor::rotate90ccw(params);
    EXPECT_EQ(result.rotation, 270);
}

TEST_F(ImageEditorTest, Rotate180)
{
    EditParams params;
    auto result = ImageEditor::rotate180(params);
    EXPECT_EQ(result.rotation, 180);
}

TEST_F(ImageEditorTest, Rotate180Chained)
{
    EditParams params;
    auto result = ImageEditor::rotate180(params);
    result = ImageEditor::rotate180(result);
    EXPECT_EQ(result.rotation, 0);
}

TEST_F(ImageEditorTest, ResizeBasic)
{
    EditParams params;
    auto result = ImageEditor::resize(params, 800, 600);
    EXPECT_EQ(result.targetWidth, 800);
    EXPECT_EQ(result.targetHeight, 600);
}

TEST_F(ImageEditorTest, ResizeNonDestructive)
{
    EditParams params;
    params.rotation = 90;
    auto result = ImageEditor::resize(params, 640, 480);
    EXPECT_EQ(result.rotation, 90);
    EXPECT_EQ(result.targetWidth, 640);
    EXPECT_EQ(result.targetHeight, 480);
}

TEST_F(ImageEditorTest, AdjustBrightnessPositive)
{
    EditParams params;
    auto result = ImageEditor::adjustBrightness(params, 15);
    EXPECT_EQ(result.brightness, 15);
}

TEST_F(ImageEditorTest, AdjustBrightnessNegative)
{
    EditParams params;
    auto result = ImageEditor::adjustBrightness(params, -10);
    EXPECT_EQ(result.brightness, -10);
}

TEST_F(ImageEditorTest, AdjustBrightnessClampedMax)
{
    EditParams params;
    auto result = ImageEditor::adjustBrightness(params, 50);
    EXPECT_EQ(result.brightness, 20);
}

TEST_F(ImageEditorTest, AdjustBrightnessClampedMin)
{
    EditParams params;
    auto result = ImageEditor::adjustBrightness(params, -50);
    EXPECT_EQ(result.brightness, -20);
}

TEST_F(ImageEditorTest, AdjustContrastNormal)
{
    EditParams params;
    auto result = ImageEditor::adjustContrast(params, 1.5f);
    EXPECT_FLOAT_EQ(result.contrast, 1.5f);
}

TEST_F(ImageEditorTest, AdjustContrastClampedHigh)
{
    EditParams params;
    auto result = ImageEditor::adjustContrast(params, 5.0f);
    EXPECT_FLOAT_EQ(result.contrast, 2.0f);
}

TEST_F(ImageEditorTest, AdjustContrastClampedLow)
{
    EditParams params;
    auto result = ImageEditor::adjustContrast(params, 0.1f);
    EXPECT_FLOAT_EQ(result.contrast, 0.5f);
}

TEST_F(ImageEditorTest, ApplyGrayscale)
{
    EditParams params;
    auto result = ImageEditor::applyGrayscale(params);
    EXPECT_EQ(result.filter, EditParams::Filter::GRAYSCALE);
}

TEST_F(ImageEditorTest, ApplySepia)
{
    EditParams params;
    auto result = ImageEditor::applySepia(params);
    EXPECT_EQ(result.filter, EditParams::Filter::SEPIA);
}

TEST_F(ImageEditorTest, ApplyVintage)
{
    EditParams params;
    auto result = ImageEditor::applyVintage(params);
    EXPECT_EQ(result.filter, EditParams::Filter::VINTAGE);
}

TEST_F(ImageEditorTest, FilterChain)
{
    EditParams params;
    auto result = ImageEditor::applyGrayscale(params);
    result = ImageEditor::applySepia(result); // Should overwrite
    EXPECT_EQ(result.filter, EditParams::Filter::SEPIA);
}

TEST_F(ImageEditorTest, HasEditsNoEdits)
{
    EditParams params;
    EXPECT_FALSE(params.hasEdits());
}

TEST_F(ImageEditorTest, HasEditsWithCrop)
{
    EditParams params;
    params.cropWidth = 100;
    EXPECT_TRUE(params.hasEdits());
}

TEST_F(ImageEditorTest, HasEditsWithRotation)
{
    EditParams params;
    params.rotation = 90;
    EXPECT_TRUE(params.hasEdits());
}

TEST_F(ImageEditorTest, HasEditsWithResize)
{
    EditParams params;
    params.targetWidth = 800;
    EXPECT_TRUE(params.hasEdits());
}

TEST_F(ImageEditorTest, HasEditsWithBrightness)
{
    EditParams params;
    params.brightness = 5;
    EXPECT_TRUE(params.hasEdits());
}

TEST_F(ImageEditorTest, HasEditsWithContrast)
{
    EditParams params;
    params.contrast = 1.5f;
    EXPECT_TRUE(params.hasEdits());
}

TEST_F(ImageEditorTest, HasEditsWithFilter)
{
    EditParams params;
    params.filter = EditParams::Filter::GRAYSCALE;
    EXPECT_TRUE(params.hasEdits());
}

TEST_F(ImageEditorTest, ToJsonBasic)
{
    EditParams params;
    params.cropX = 10;
    params.cropY = 20;
    params.cropWidth = 100;
    params.cropHeight = 80;
    params.rotation = 90;
    params.brightness = 5;
    params.contrast = 1.5f;
    params.filter = EditParams::Filter::GRAYSCALE;

    std::string json = ImageEditor::toJson(params);
    EXPECT_NE(json.find("\"cropX\":10"), std::string::npos);
    EXPECT_NE(json.find("\"cropY\":20"), std::string::npos);
    EXPECT_NE(json.find("\"cropWidth\":100"), std::string::npos);
    EXPECT_NE(json.find("\"rotation\":90"), std::string::npos);
    EXPECT_NE(json.find("\"brightness\":5"), std::string::npos);
    EXPECT_NE(json.find("\"filter\":1"), std::string::npos);
}

TEST_F(ImageEditorTest, FromJsonBasic)
{
    std::string json = "{\"cropX\":10,\"cropY\":20,\"cropWidth\":100,\"cropHeight\":80,"
                       "\"rotation\":90,\"brightness\":5,\"filter\":1}";

    auto params = ImageEditor::fromJson(json);
    ASSERT_TRUE(params.has_value());
    EXPECT_EQ(params->cropX, 10);
    EXPECT_EQ(params->cropY, 20);
    EXPECT_EQ(params->cropWidth, 100);
    EXPECT_EQ(params->cropHeight, 80);
    EXPECT_EQ(params->rotation, 90);
    EXPECT_EQ(params->brightness, 5);
    EXPECT_EQ(params->filter, EditParams::Filter::GRAYSCALE);
}

TEST_F(ImageEditorTest, RoundTripJson)
{
    EditParams original;
    original.cropX = 5;
    original.cropY = 10;
    original.cropWidth = 200;
    original.cropHeight = 150;
    original.rotation = 180;
    original.targetWidth = 640;
    original.targetHeight = 480;
    original.brightness = -10;
    original.contrast = 1.8f;
    original.filter = EditParams::Filter::SEPIA;

    std::string json = ImageEditor::toJson(original);
    auto parsed = ImageEditor::fromJson(json);
    ASSERT_TRUE(parsed.has_value());

    EXPECT_EQ(parsed->cropX, original.cropX);
    EXPECT_EQ(parsed->cropY, original.cropY);
    EXPECT_EQ(parsed->cropWidth, original.cropWidth);
    EXPECT_EQ(parsed->cropHeight, original.cropHeight);
    EXPECT_EQ(parsed->rotation, original.rotation);
    EXPECT_EQ(parsed->targetWidth, original.targetWidth);
    EXPECT_EQ(parsed->targetHeight, original.targetHeight);
    EXPECT_EQ(parsed->brightness, original.brightness);
    EXPECT_FLOAT_EQ(parsed->contrast, original.contrast);
    EXPECT_EQ(parsed->filter, original.filter);
}

TEST_F(ImageEditorTest, ChainedOperations)
{
    EditParams params;
    // Apply multiple operations in sequence
    params = ImageEditor::crop(params, 10, 10, 200, 200);
    params = ImageEditor::rotate90cw(params);
    params = ImageEditor::resize(params, 800, 600);
    params = ImageEditor::adjustBrightness(params, 10);
    params = ImageEditor::adjustContrast(params, 1.2f);
    params = ImageEditor::applySepia(params);

    EXPECT_EQ(params.cropX, 10);
    EXPECT_EQ(params.cropY, 10);
    EXPECT_EQ(params.cropWidth, 200);
    EXPECT_EQ(params.cropHeight, 200);
    EXPECT_EQ(params.rotation, 90);
    EXPECT_EQ(params.targetWidth, 800);
    EXPECT_EQ(params.targetHeight, 600);
    EXPECT_EQ(params.brightness, 10);
    EXPECT_FLOAT_EQ(params.contrast, 1.2f);
    EXPECT_EQ(params.filter, EditParams::Filter::SEPIA);
}

TEST_F(ImageEditorTest, ApplyEditsNoEdits)
{
    // Create a simple 2x2 RGB image (24 bytes)
    std::vector<unsigned char> imageData = {
        255, 0,   0,   // Red pixel
        0,   255, 0,   // Green pixel
        0,   0,   255, // Blue pixel
        255, 255, 0    // Yellow pixel
    };

    EditParams params;
    params.originalWidth = 2;
    params.originalHeight = 2;

    auto result = ImageEditor::applyEdits(imageData, params);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), imageData.size());
}

TEST_F(ImageEditorTest, ApplyEditsInvalidCrop)
{
    std::vector<unsigned char> imageData(12, 0);
    EditParams params;
    params.originalWidth = 2;
    params.originalHeight = 2;
    params.cropX = 5;
    params.cropWidth = 10;
    params.cropHeight = 10;

    auto result = ImageEditor::applyEdits(imageData, params);
    EXPECT_FALSE(result.has_value());
}

TEST_F(ImageEditorTest, EditorPlaceholder) { EXPECT_TRUE(true); }

} // namespace image

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}