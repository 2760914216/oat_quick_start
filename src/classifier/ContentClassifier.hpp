#ifndef CONTENT_CLASSIFIER_HPP
#define CONTENT_CLASSIFIER_HPP

#include "ImageClassifier.hpp"
#include <memory>
#include <string>
#include <vector>

// Forward declaration for ONNX types
#if ENABLE_ONNX
namespace Ort
{
class Env;
class Session;
class SessionOptions;
} // namespace Ort
#endif

/**
 * @brief ONNX-based content classifier
 *
 * Loads and runs ONNX model for content classification.
 * Supports 6 content categories: 人物, 风景, 建筑, 产品, 文档, 其他
 */
class ContentClassifier : public ImageClassifier
{
public:
    /**
     * @brief Constructor
     * @param modelPath Path to ONNX model file
     */
    explicit ContentClassifier(const std::string& modelPath);

    /**
     * @brief Destructor
     */
    ~ContentClassifier() override;

    /**
     * @brief Classify image content
     * @param imageData Raw image data as RGB/RGBA bytes
     * @return ClassificationResult with predicted content category
     */
    ClassificationResult classify(const std::vector<unsigned char>& imageData) override;

    /**
     * @brief Check if model is loaded
     * @return true if model is ready for inference
     */
    bool isModelLoaded() const override;

    /**
     * @brief Unload model from memory
     */
    void unloadModel() override;

    /**
     * @brief Get model type
     * @return "onnx" for ONNX model
     */
    std::string getModelType() const override;

private:
    /// Model file path
    std::string modelPath_;

    /// Lazy loading flag
    bool modelLoaded_ = false;

    /// Error message when ONNX not available
    std::string errorMessage_;

#if ENABLE_ONNX
    /// ONNX Runtime environment
    std::unique_ptr<Ort::Env> env_;

    /// ONNX Runtime session
    std::unique_ptr<Ort::Session> session_;

    /// Session options
    std::unique_ptr<Ort::SessionOptions> sessionOptions_;
#endif

    /**
     * @brief Load ONNX model (lazy loading)
     */
    void loadModel();

    /**
     * @brief Run ONNX inference
     * @param imageData Input image data
     * @return ClassificationResult
     */
    ClassificationResult runInference(const std::vector<unsigned char>& imageData);

    /**
     * @brief Preprocess image data for model input
     * @param imageData Raw image data
     * @return Preprocessed float tensor data
     */
    std::vector<float> preprocessImage(const std::vector<unsigned char>& imageData) const;

    /**
     * @brief Postprocess model output to get classification result
     * @param outputData Model output tensor
     * @return ClassificationResult
     */
    ClassificationResult postprocessOutput(const float* outputData) const;
};

#endif // CONTENT_CLASSIFIER_HPP