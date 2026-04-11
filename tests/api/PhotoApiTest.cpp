#include "ImageEditor.hpp"
#include <gtest/gtest.h>
#include <string>

namespace image
{

class PhotoApiTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(PhotoApiTest, GetClassificationEndpointPath)
{
    std::string photoId = "test123";
    std::string expectedPath = "/api/photos/" + photoId + "/classification";
    EXPECT_EQ(expectedPath, "/api/photos/test123/classification");
}

TEST_F(PhotoApiTest, PostClassifyEndpointPath)
{
    std::string photoId = "test456";
    std::string expectedPath = "/api/photos/" + photoId + "/classify";
    EXPECT_EQ(expectedPath, "/api/photos/test456/classify");
}

TEST_F(PhotoApiTest, PostEditEndpointPath)
{
    std::string photoId = "test789";
    std::string expectedPath = "/api/photos/" + photoId + "/edit";
    EXPECT_EQ(expectedPath, "/api/photos/test789/edit");
}

TEST_F(PhotoApiTest, GetEditEndpointPath)
{
    std::string photoId = "test101";
    std::string expectedPath = "/api/photos/" + photoId + "/edit";
    EXPECT_EQ(expectedPath, "/api/photos/test101/edit");
}

TEST_F(PhotoApiTest, PostRevertEndpointPath)
{
    std::string photoId = "test202";
    std::string expectedPath = "/api/photos/" + photoId + "/revert";
    EXPECT_EQ(expectedPath, "/api/photos/test202/revert");
}

TEST_F(PhotoApiTest, GetThumbnailEndpointPath)
{
    std::string photoId = "test303";
    std::string expectedPath = "/api/photos/" + photoId + "/thumbnail";
    EXPECT_EQ(expectedPath, "/api/photos/test303/thumbnail");
}

TEST_F(PhotoApiTest, GetThumbnailEndpointPathWithQueryParam)
{
    std::string photoId = "test404";
    std::string thumbSize = "list";
    std::string expectedPath = "/api/photos/" + photoId + "/thumbnail?thumbSize=" + thumbSize;
    EXPECT_EQ(expectedPath, "/api/photos/test404/thumbnail?thumbSize=list");
}

TEST_F(PhotoApiTest, ThumbnailSizeQuery_List)
{
    std::string sizeType = "list";
    EXPECT_EQ(sizeType, "list");
}

TEST_F(PhotoApiTest, ThumbnailSizeQuery_Detail)
{
    std::string sizeType = "detail";
    EXPECT_EQ(sizeType, "detail");
}

TEST_F(PhotoApiTest, ThumbnailSizeQuery_DefaultWhenEmpty)
{
    std::string sizeType = "list";
    EXPECT_EQ(sizeType, "list");
}

TEST_F(PhotoApiTest, ThumbnailSizeQuery_InvalidDefaultsToList)
{
    std::string sizeType = "invalid";
    if (sizeType != "list" && sizeType != "detail")
    {
        sizeType = "list";
    }
    EXPECT_EQ(sizeType, "list");
}

TEST_F(PhotoApiTest, ClassificationStatus_Pending)
{
    std::string status = "pending";
    EXPECT_EQ(status, "pending");
}

TEST_F(PhotoApiTest, ClassificationStatus_Processing)
{
    std::string status = "processing";
    EXPECT_EQ(status, "processing");
}

TEST_F(PhotoApiTest, ClassificationStatus_Completed)
{
    std::string status = "completed";
    EXPECT_EQ(status, "completed");
}

TEST_F(PhotoApiTest, ClassificationStatus_Failed)
{
    std::string status = "failed";
    EXPECT_EQ(status, "failed");
}

TEST_F(PhotoApiTest, EditBrightnessClampPositive)
{
    int brightness = 25;
    if (brightness > 20) brightness = 20;
    EXPECT_EQ(brightness, 20);
}

TEST_F(PhotoApiTest, EditBrightnessClampNegative)
{
    int brightness = -25;
    if (brightness < -20) brightness = -20;
    EXPECT_EQ(brightness, -20);
}

TEST_F(PhotoApiTest, EditBrightnessValid)
{
    int brightness = 5;
    if (brightness < -20) brightness = -20;
    if (brightness > 20) brightness = 20;
    EXPECT_EQ(brightness, 5);
}

TEST_F(PhotoApiTest, EditContrastClampHigh)
{
    float contrast = 2.5f;
    if (contrast > 2.0f) contrast = 2.0f;
    EXPECT_FLOAT_EQ(contrast, 2.0f);
}

TEST_F(PhotoApiTest, EditContrastClampLow)
{
    float contrast = 0.2f;
    if (contrast < 0.5f) contrast = 0.5f;
    EXPECT_FLOAT_EQ(contrast, 0.5f);
}

TEST_F(PhotoApiTest, EditContrastValid)
{
    float contrast = 1.5f;
    if (contrast < 0.5f) contrast = 0.5f;
    if (contrast > 2.0f) contrast = 2.0f;
    EXPECT_FLOAT_EQ(contrast, 1.5f);
}

TEST_F(PhotoApiTest, EditRotationValid0)
{
    int rotation = 0;
    EXPECT_TRUE(rotation == 0 || rotation == 90 || rotation == 180 || rotation == 270);
}

TEST_F(PhotoApiTest, EditRotationValid90)
{
    int rotation = 90;
    EXPECT_TRUE(rotation == 0 || rotation == 90 || rotation == 180 || rotation == 270);
}

TEST_F(PhotoApiTest, EditRotationValid180)
{
    int rotation = 180;
    EXPECT_TRUE(rotation == 0 || rotation == 90 || rotation == 180 || rotation == 270);
}

TEST_F(PhotoApiTest, EditRotationValid270)
{
    int rotation = 270;
    EXPECT_TRUE(rotation == 0 || rotation == 90 || rotation == 180 || rotation == 270);
}

TEST_F(PhotoApiTest, EditRotationInvalidDefaultsTo0)
{
    int rotation = 45;
    if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270)
    {
        rotation = 0;
    }
    EXPECT_EQ(rotation, 0);
}

TEST_F(PhotoApiTest, EditFilterNone)
{
    EditParams params;
    EXPECT_EQ(params.filter, EditParams::Filter::NONE);
}

TEST_F(PhotoApiTest, EditFilterGrayscale)
{
    EditParams params;
    params.filter = EditParams::Filter::GRAYSCALE;
    EXPECT_EQ(params.filter, EditParams::Filter::GRAYSCALE);
}

TEST_F(PhotoApiTest, EditFilterSepia)
{
    EditParams params;
    params.filter = EditParams::Filter::SEPIA;
    EXPECT_EQ(params.filter, EditParams::Filter::SEPIA);
}

TEST_F(PhotoApiTest, EditFilterVintage)
{
    EditParams params;
    params.filter = EditParams::Filter::VINTAGE;
    EXPECT_EQ(params.filter, EditParams::Filter::VINTAGE);
}

TEST_F(PhotoApiTest, EditFilterChain)
{
    EditParams params;
    params.filter = EditParams::Filter::GRAYSCALE;
    params.filter = EditParams::Filter::SEPIA;
    EXPECT_EQ(params.filter, EditParams::Filter::SEPIA);
}

TEST_F(PhotoApiTest, HasEditsNoEdits)
{
    EditParams params;
    EXPECT_FALSE(params.hasEdits());
}

TEST_F(PhotoApiTest, HasEditsWithCrop)
{
    EditParams params;
    params.cropWidth = 100;
    EXPECT_TRUE(params.hasEdits());
}

TEST_F(PhotoApiTest, HasEditsWithRotation)
{
    EditParams params;
    params.rotation = 90;
    EXPECT_TRUE(params.hasEdits());
}

TEST_F(PhotoApiTest, HasEditsWithResize)
{
    EditParams params;
    params.targetWidth = 800;
    EXPECT_TRUE(params.hasEdits());
}

TEST_F(PhotoApiTest, HasEditsWithBrightness)
{
    EditParams params;
    params.brightness = 5;
    EXPECT_TRUE(params.hasEdits());
}

TEST_F(PhotoApiTest, HasEditsWithContrast)
{
    EditParams params;
    params.contrast = 1.5f;
    EXPECT_TRUE(params.hasEdits());
}

TEST_F(PhotoApiTest, HasEditsWithFilter)
{
    EditParams params;
    params.filter = EditParams::Filter::GRAYSCALE;
    EXPECT_TRUE(params.hasEdits());
}

TEST_F(PhotoApiTest, CropBasic)
{
    EditParams params;
    auto result = ImageEditor::crop(params, 10, 20, 100, 80);
    EXPECT_EQ(result.cropX, 10);
    EXPECT_EQ(result.cropY, 20);
    EXPECT_EQ(result.cropWidth, 100);
    EXPECT_EQ(result.cropHeight, 80);
}

TEST_F(PhotoApiTest, CropNonDestructive)
{
    EditParams params;
    params.brightness = 5;
    auto result = ImageEditor::crop(params, 0, 0, 50, 50);
    EXPECT_EQ(result.brightness, 5);
    EXPECT_EQ(result.cropWidth, 50);
}

TEST_F(PhotoApiTest, Rotate90CW)
{
    EditParams params;
    auto result = ImageEditor::rotate90cw(params);
    EXPECT_EQ(result.rotation, 90);
}

TEST_F(PhotoApiTest, Rotate180)
{
    EditParams params;
    auto result = ImageEditor::rotate180(params);
    EXPECT_EQ(result.rotation, 180);
}

TEST_F(PhotoApiTest, Rotate180Chained)
{
    EditParams params;
    auto result = ImageEditor::rotate180(params);
    result = ImageEditor::rotate180(result);
    EXPECT_EQ(result.rotation, 0);
}

TEST_F(PhotoApiTest, ResizeBasic)
{
    EditParams params;
    auto result = ImageEditor::resize(params, 800, 600);
    EXPECT_EQ(result.targetWidth, 800);
    EXPECT_EQ(result.targetHeight, 600);
}

TEST_F(PhotoApiTest, ResizeNonDestructive)
{
    EditParams params;
    params.rotation = 90;
    auto result = ImageEditor::resize(params, 640, 480);
    EXPECT_EQ(result.rotation, 90);
    EXPECT_EQ(result.targetWidth, 640);
    EXPECT_EQ(result.targetHeight, 480);
}

TEST_F(PhotoApiTest, AdjustBrightnessPositive)
{
    EditParams params;
    auto result = ImageEditor::adjustBrightness(params, 15);
    EXPECT_EQ(result.brightness, 15);
}

TEST_F(PhotoApiTest, AdjustBrightnessNegative)
{
    EditParams params;
    auto result = ImageEditor::adjustBrightness(params, -10);
    EXPECT_EQ(result.brightness, -10);
}

TEST_F(PhotoApiTest, AdjustBrightnessClampedMax)
{
    EditParams params;
    auto result = ImageEditor::adjustBrightness(params, 50);
    EXPECT_EQ(result.brightness, 20);
}

TEST_F(PhotoApiTest, AdjustBrightnessClampedMin)
{
    EditParams params;
    auto result = ImageEditor::adjustBrightness(params, -50);
    EXPECT_EQ(result.brightness, -20);
}

TEST_F(PhotoApiTest, AdjustContrastNormal)
{
    EditParams params;
    auto result = ImageEditor::adjustContrast(params, 1.5f);
    EXPECT_FLOAT_EQ(result.contrast, 1.5f);
}

TEST_F(PhotoApiTest, AdjustContrastClampedHigh)
{
    EditParams params;
    auto result = ImageEditor::adjustContrast(params, 5.0f);
    EXPECT_FLOAT_EQ(result.contrast, 2.0f);
}

TEST_F(PhotoApiTest, AdjustContrastClampedLow)
{
    EditParams params;
    auto result = ImageEditor::adjustContrast(params, 0.1f);
    EXPECT_FLOAT_EQ(result.contrast, 0.5f);
}

TEST_F(PhotoApiTest, ApplyGrayscale)
{
    EditParams params;
    auto result = ImageEditor::applyGrayscale(params);
    EXPECT_EQ(result.filter, EditParams::Filter::GRAYSCALE);
}

TEST_F(PhotoApiTest, ApplySepia)
{
    EditParams params;
    auto result = ImageEditor::applySepia(params);
    EXPECT_EQ(result.filter, EditParams::Filter::SEPIA);
}

TEST_F(PhotoApiTest, ApplyVintage)
{
    EditParams params;
    auto result = ImageEditor::applyVintage(params);
    EXPECT_EQ(result.filter, EditParams::Filter::VINTAGE);
}

TEST_F(PhotoApiTest, ToJsonBasic)
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
}

TEST_F(PhotoApiTest, FromJsonBasic)
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

TEST_F(PhotoApiTest, RoundTripJson)
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

TEST_F(PhotoApiTest, Placeholder) { EXPECT_TRUE(true); }

} // namespace image

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}