#ifndef CLASSIFIER_MANAGER_HPP
#define CLASSIFIER_MANAGER_HPP

#include "ImageClassifier.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Manager class for multiple image classifiers
 *
 * Provides singleton access to manage multiple classifier instances.
 * Supports registering, retrieving, and running classification across all classifiers.
 */
class ClassifierManager
{
public:
    /**
     * @brief Get singleton instance
     * @return Reference to ClassifierManager instance
     */
    static ClassifierManager& getInstance();

    /**
     * @brief Register a classifier
     * @param type Unique identifier for the classifier type
     * @param classifier Shared pointer to the classifier
     */
    void registerClassifier(const std::string& type, std::shared_ptr<ImageClassifier> classifier);

    /**
     * @brief Get a classifier by type
     * @param type Classifier type identifier
     * @return Shared pointer to the classifier, or nullptr if not found
     */
    std::shared_ptr<ImageClassifier> getClassifier(const std::string& type);

    /**
     * @brief Run classification on all registered classifiers
     * @param imageData Raw image data as RGB/RGBA bytes
     * @return Vector of ClassificationResult from all classifiers
     */
    std::vector<ClassificationResult> classifyAll(const std::vector<unsigned char>& imageData);

    /**
     * @brief Set async mode
     * @param async True for async mode, false for sync mode
     */
    void setAsyncMode(bool async);

    /**
     * @brief Check if async mode is enabled
     * @return true if async mode is enabled
     */
    bool isAsyncMode() const;

    // Prevent copying
    ClassifierManager(const ClassifierManager&) = delete;
    ClassifierManager& operator=(const ClassifierManager&) = delete;

private:
    /// Private constructor for singleton
    ClassifierManager() = default;

    /// Map of classifier type to classifier instance
    std::map<std::string, std::shared_ptr<ImageClassifier>> classifiers_;

    /// Async mode flag
    bool asyncMode_ = false;
};

#endif // CLASSIFIER_MANAGER_HPP