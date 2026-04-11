// Unit tests for ImageClassifier base class and ONNX implementations

#include "classifier/ImageClassifier.hpp"
#include "classifier/ClassificationResult.hpp"
#include "classifier/ClassifierManager.hpp"
#include "classifier/ContentClassifier.hpp"
#include "classifier/SceneClassifier.hpp"

#include <gtest/gtest.h>

namespace image
{

// ============================================================================
// ClassificationResult Tests
// ============================================================================

class ClassificationResultTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ClassificationResultTest, DefaultConstructor)
{
    ClassificationResult result;
    EXPECT_EQ(result.confidence, 0.0f);
    EXPECT_TRUE(result.category.empty());
    EXPECT_TRUE(result.allProbabilities.empty());
    EXPECT_TRUE(result.modelName.empty());
    EXPECT_TRUE(result.timestamp.empty());
}

TEST_F(ClassificationResultTest, FieldAssignment)
{
    ClassificationResult result;
    result.category = "会议";
    result.confidence = 0.95f;
    result.allProbabilities = {0.95f, 0.03f, 0.02f};
    result.modelName = "scene_classifier";
    result.timestamp = "2024-01-01T00:00:00";

    EXPECT_EQ(result.category, "会议");
    EXPECT_FLOAT_EQ(result.confidence, 0.95f);
    EXPECT_EQ(result.allProbabilities.size(), 3);
    EXPECT_EQ(result.modelName, "scene_classifier");
    EXPECT_EQ(result.timestamp, "2024-01-01T00:00:00");
}

TEST_F(ClassificationResultTest, ToJsonEmpty)
{
    ClassificationResult result;
    std::string json = result.toJson();

    EXPECT_TRUE(json.find("\"category\":\"\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"confidence\":0.0000") != std::string::npos);
    EXPECT_TRUE(json.find("\"allProbabilities\":[]") != std::string::npos);
    EXPECT_TRUE(json.find("\"modelName\":\"\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"timestamp\":\"\"") != std::string::npos);
}

TEST_F(ClassificationResultTest, ToJsonWithData)
{
    ClassificationResult result;
    result.category = "风景";
    result.confidence = 0.87f;
    result.allProbabilities = {0.87f, 0.10f, 0.03f};
    result.modelName = "content_classifier";
    result.timestamp = "2024-06-15T10:30:00";

    std::string json = result.toJson();

    EXPECT_TRUE(json.find("\"category\":\"风景\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"confidence\":0.8700") != std::string::npos);
    EXPECT_TRUE(json.find("\"allProbabilities\":[0.8700,0.1000,0.0300]") != std::string::npos);
    EXPECT_TRUE(json.find("\"modelName\":\"content_classifier\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"timestamp\":\"2024-06-15T10:30:00\"") != std::string::npos);
}

TEST_F(ClassificationResultTest, ToJsonFormat)
{
    ClassificationResult result;
    result.category = "人物";
    result.confidence = 0.5f;

    std::string json = result.toJson();

    // Verify JSON structure: starts with { and ends with }
    EXPECT_EQ(json.front(), '{');
    EXPECT_EQ(json.back(), '}');
}

// ============================================================================
// ClassifierManager Singleton Tests
// ============================================================================

class ClassifierManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Clear any registered classifiers before each test
        // Note: We can't directly clear since there's no clear method,
        // but we use unique type names for each test
    }

    void TearDown() override {}
};

TEST_F(ClassifierManagerTest, SingletonReturnsSameInstance)
{
    ClassifierManager& instance1 = ClassifierManager::getInstance();
    ClassifierManager& instance2 = ClassifierManager::getInstance();

    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(ClassifierManagerTest, SingletonMultipleCalls)
{
    ClassifierManager& instance1 = ClassifierManager::getInstance();
    ClassifierManager& instance2 = ClassifierManager::getInstance();
    ClassifierManager& instance3 = ClassifierManager::getInstance();

    EXPECT_EQ(&instance1, &instance2);
    EXPECT_EQ(&instance2, &instance3);
}

TEST_F(ClassifierManagerTest, RegisterAndGetClassifier)
{
    ClassifierManager& manager = ClassifierManager::getInstance();

    // Create a mock classifier using concrete SceneClassifier (no model needed)
    auto classifier = std::make_shared<SceneClassifier>("dummy_model.onnx");

    manager.registerClassifier("test_scene", classifier);

    auto retrieved = manager.getClassifier("test_scene");
    EXPECT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved.get(), classifier.get());
}

TEST_F(ClassifierManagerTest, GetNonExistentClassifier)
{
    ClassifierManager& manager = ClassifierManager::getInstance();

    auto result = manager.getClassifier("nonexistent_type");
    EXPECT_EQ(result, nullptr);
}

TEST_F(ClassifierManagerTest, RegisterMultipleClassifiers)
{
    ClassifierManager& manager = ClassifierManager::getInstance();

    auto sceneClassifier = std::make_shared<SceneClassifier>("scene.onnx");
    auto contentClassifier = std::make_shared<ContentClassifier>("content.onnx");

    manager.registerClassifier("scene", sceneClassifier);
    manager.registerClassifier("content", contentClassifier);

    auto retrievedScene = manager.getClassifier("scene");
    auto retrievedContent = manager.getClassifier("content");

    EXPECT_NE(retrievedScene, nullptr);
    EXPECT_NE(retrievedContent, nullptr);
    EXPECT_EQ(retrievedScene.get(), sceneClassifier.get());
    EXPECT_EQ(retrievedContent.get(), contentClassifier.get());
}

// ============================================================================
// ClassifierManager Async Mode Tests
// ============================================================================

TEST_F(ClassifierManagerTest, DefaultAsyncModeIsFalse)
{
    ClassifierManager& manager = ClassifierManager::getInstance();
    EXPECT_FALSE(manager.isAsyncMode());
}

TEST_F(ClassifierManagerTest, SetAsyncMode)
{
    ClassifierManager& manager = ClassifierManager::getInstance();

    manager.setAsyncMode(true);
    EXPECT_TRUE(manager.isAsyncMode());

    manager.setAsyncMode(false);
    EXPECT_FALSE(manager.isAsyncMode());
}

// ============================================================================
// Lazy Loading Tests
// ============================================================================

class LazyLoadingTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LazyLoadingTest, SceneClassifierModelNotLoadedInitially)
{
    SceneClassifier classifier("dummy_path.onnx");
    EXPECT_FALSE(classifier.isModelLoaded());
}

TEST_F(LazyLoadingTest, ContentClassifierModelNotLoadedInitially)
{
    ContentClassifier classifier("dummy_path.onnx");
    EXPECT_FALSE(classifier.isModelLoaded());
}

TEST_F(LazyLoadingTest, SceneClassifierGetModelType)
{
    SceneClassifier classifier("dummy_path.onnx");
    EXPECT_EQ(classifier.getModelType(), "onnx");
}

TEST_F(LazyLoadingTest, ContentClassifierGetModelType)
{
    ContentClassifier classifier("dummy_path.onnx");
    EXPECT_EQ(classifier.getModelType(), "onnx");
}

TEST_F(LazyLoadingTest, SceneClassifierUnloadModel)
{
    SceneClassifier classifier("dummy_path.onnx");
    // Model should not be loaded, unload should handle gracefully
    classifier.unloadModel();
    EXPECT_FALSE(classifier.isModelLoaded());
}

TEST_F(LazyLoadingTest, ContentClassifierUnloadModel)
{
    ContentClassifier classifier("dummy_path.onnx");
    // Model should not be loaded, unload should handle gracefully
    classifier.unloadModel();
    EXPECT_FALSE(classifier.isModelLoaded());
}

// ============================================================================
// Error Handling Tests
// ============================================================================

class ErrorHandlingTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ErrorHandlingTest, ClassifyWithoutModelLoaded)
{
    SceneClassifier classifier("nonexistent.onnx");

    EXPECT_FALSE(classifier.isModelLoaded());

    std::vector<unsigned char> emptyImageData;
    ClassificationResult result = classifier.classify(emptyImageData);

    EXPECT_EQ(result.category, "其他");
    EXPECT_FLOAT_EQ(result.confidence, 0.0f);
}

TEST_F(ErrorHandlingTest, ContentClassifierClassifyWithoutModel)
{
    ContentClassifier classifier("nonexistent.onnx");

    EXPECT_FALSE(classifier.isModelLoaded());

    std::vector<unsigned char> emptyImageData;
    ClassificationResult result = classifier.classify(emptyImageData);

    EXPECT_EQ(result.category, "其他");
    EXPECT_FLOAT_EQ(result.confidence, 0.0f);
}

TEST_F(ErrorHandlingTest, SceneClassifierDestructor)
{
    auto classifier = std::make_shared<SceneClassifier>("test.onnx");
    EXPECT_FALSE(classifier->isModelLoaded());
    // Destructor should be called without issues when shared_ptr goes out of scope
}

TEST_F(ErrorHandlingTest, ContentClassifierDestructor)
{
    auto classifier = std::make_shared<ContentClassifier>("test.onnx");
    EXPECT_FALSE(classifier->isModelLoaded());
    // Destructor should be called without issues when shared_ptr goes out of scope
}

// ============================================================================
// ImageClassifier Interface Tests
// ============================================================================

class ImageClassifierInterfaceTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ImageClassifierInterfaceTest, SceneClassifierImplementsInterface)
{
    std::shared_ptr<ImageClassifier> classifier = std::make_shared<SceneClassifier>("test.onnx");

    // Verify interface methods are accessible through base class pointer
    EXPECT_FALSE(classifier->isModelLoaded());
    EXPECT_EQ(classifier->getModelType(), "onnx");

    classifier->unloadModel();
    EXPECT_FALSE(classifier->isModelLoaded());
}

TEST_F(ImageClassifierInterfaceTest, ContentClassifierImplementsInterface)
{
    std::shared_ptr<ImageClassifier> classifier = std::make_shared<ContentClassifier>("test.onnx");

    // Verify interface methods are accessible through base class pointer
    EXPECT_FALSE(classifier->isModelLoaded());
    EXPECT_EQ(classifier->getModelType(), "onnx");

    classifier->unloadModel();
    EXPECT_FALSE(classifier->isModelLoaded());
}

TEST_F(ImageClassifierInterfaceTest, ClassifierStoredInManagerViaBasePointer)
{
    ClassifierManager& manager = ClassifierManager::getInstance();

    // Store through base class pointer
    std::shared_ptr<ImageClassifier> sceneClassifier = std::make_shared<SceneClassifier>("scene.onnx");
    manager.registerClassifier("scene_interface_test", sceneClassifier);

    // Retrieve and verify
    auto retrieved = manager.getClassifier("scene_interface_test");
    EXPECT_NE(retrieved, nullptr);
    EXPECT_FALSE(retrieved->isModelLoaded());
    EXPECT_EQ(retrieved->getModelType(), "onnx");
}

} // namespace image

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}