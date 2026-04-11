#include "ClassifierManager.hpp"
#include "ClassificationResult.hpp"

ClassifierManager& ClassifierManager::getInstance()
{
    static ClassifierManager instance;
    return instance;
}

void ClassifierManager::registerClassifier(const std::string& type, std::shared_ptr<ImageClassifier> classifier) { classifiers_[type] = classifier; }

std::shared_ptr<ImageClassifier> ClassifierManager::getClassifier(const std::string& type)
{
    auto it = classifiers_.find(type);
    if (it != classifiers_.end())
    {
        return it->second;
    }
    return nullptr;
}

std::vector<ClassificationResult> ClassifierManager::classifyAll(const std::vector<unsigned char>& imageData)
{
    std::vector<ClassificationResult> results;
    for (const auto& pair : classifiers_)
    {
        auto classifier = pair.second;
        if (classifier && classifier->isModelLoaded())
        {
            results.push_back(classifier->classify(imageData));
        }
    }
    return results;
}

void ClassifierManager::setAsyncMode(bool async) { asyncMode_ = async; }

bool ClassifierManager::isAsyncMode() const { return asyncMode_; }