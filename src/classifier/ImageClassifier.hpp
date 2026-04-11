#ifndef IMAGE_CLASSIFIER_HPP
#define IMAGE_CLASSIFIER_HPP

#include <string>
#include <vector>

// Forward declaration - will be defined in ClassificationResult
struct ClassificationResult;

/**
 * @brief Abstract base class for image classification
 *
 * Defines the interface for image classifiers.
 * Concrete implementations (e.g., ONNXClassifier) will inherit from this class
 * and implement the classification logic.
 */
class ImageClassifier
{
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ImageClassifier() = default;

    /**
     * @brief Classify an image
     *
     * @param imageData Raw image data as RGB/RGBA bytes
     * @return ClassificationResult containing predicted class and confidence
     */
    virtual ClassificationResult classify(const std::vector<unsigned char>& imageData) = 0;

    /**
     * @brief Check if the model is loaded
     *
     * @return true if model is loaded and ready for inference
     */
    virtual bool isModelLoaded() const = 0;

    /**
     * @brief Unload the model from memory
     */
    virtual void unloadModel() = 0;

    /**
     * @brief Get the model type
     *
     * @return String identifying the model type (e.g., "onnx", "torch")
     */
    virtual std::string getModelType() const = 0;
};

#endif // IMAGE_CLASSIFIER_HPP