#include "ContentClassifier.hpp"
#include "ClassificationResult.hpp"
#include <ctime>
#include <fstream>
#include <sstream>

#if ENABLE_ONNX
#include <onnxruntime_cxx_api.h>
#endif

namespace
{

std::string get_current_timestamp()
{
    std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
    return buf;
}

const std::vector<std::string> CONTENT_CATEGORIES = {"人物", "风景", "建筑", "产品", "文档", "其他"};

const int MODEL_INPUT_WIDTH = 224;
const int MODEL_INPUT_HEIGHT = 224;
const int MODEL_INPUT_CHANNELS = 3;
} // namespace

#if ENABLE_ONNX
ContentClassifier::ContentClassifier(const std::string& modelPath) : modelPath_(modelPath), modelLoaded_(false) {}

ContentClassifier::~ContentClassifier() = default;

void ContentClassifier::loadModel()
{
    if (modelLoaded_)
    {
        return;
    }

    try
    {
        env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ContentClassifier");
        sessionOptions_ = std::make_unique<Ort::SessionOptions>();
        sessionOptions_->SetIntraOpNumThreads(4);
        sessionOptions_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        session_ = std::make_unique<Ort::Session>(*env_, modelPath_.c_str(), *sessionOptions_);
        modelLoaded_ = true;
    }
    catch (const std::exception& e)
    {
        errorMessage_ = std::string("Failed to load ONNX model: ") + e.what();
        modelLoaded_ = false;
    }
}

ClassificationResult ContentClassifier::runInference(const std::vector<unsigned char>& imageData)
{
    if (!modelLoaded_)
    {
        loadModel();
    }

    if (!modelLoaded_)
    {
        ClassificationResult result;
        result.category = "其他";
        result.confidence = 0.0f;
        result.allProbabilities = std::vector<float>(CONTENT_CATEGORIES.size(), 0.0f);
        result.modelName = "onnx";
        result.timestamp = get_current_timestamp();
        return result;
    }

    auto inputData = preprocessImage(imageData);
    std::vector<int64_t> inputShape = {1, MODEL_INPUT_CHANNELS, MODEL_INPUT_HEIGHT, MODEL_INPUT_WIDTH};

    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memoryInfo, inputData.data(), inputData.size(), inputShape.data(), inputShape.size());

    Ort::AllocatorWithDefaultOptions allocator;
    auto inputName = session_->GetInputNameAllocated(0, allocator);
    auto outputName = session_->GetOutputNameAllocated(0, allocator);

    const char* inputNames[] = {inputName.get()};
    const char* outputNames[] = {outputName.get()};

    auto outputTensors = session_->Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);

    float* outputData = outputTensors[0].GetTensorMutableData<float>();
    return postprocessOutput(outputData);
}

std::vector<float> ContentClassifier::preprocessImage(const std::vector<unsigned char>& imageData) const
{
    std::vector<float> output(MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS, 0.0f);

    if (imageData.empty())
    {
        return output;
    }

    size_t width = MODEL_INPUT_WIDTH;
    size_t height = MODEL_INPUT_HEIGHT;
    size_t channels = MODEL_INPUT_CHANNELS;

    if (imageData.size() >= width * height * channels)
    {
        for (size_t i = 0; i < width * height; ++i)
        {
            for (size_t c = 0; c < channels; ++c)
            {
                size_t srcIdx = i * channels + c;
                size_t dstIdx = c * width * height + i;
                if (srcIdx < imageData.size())
                {
                    output[dstIdx] = static_cast<float>(imageData[srcIdx]) / 255.0f;
                }
            }
        }
    }

    return output;
}

ClassificationResult ContentClassifier::postprocessOutput(const float* outputData) const
{
    ClassificationResult result;
    result.modelName = "onnx";
    result.timestamp = get_current_timestamp();
    result.allProbabilities.resize(CONTENT_CATEGORIES.size());

    float maxProb = 0.0f;
    int maxIdx = 0;

    for (size_t i = 0; i < CONTENT_CATEGORIES.size(); ++i)
    {
        result.allProbabilities[i] = outputData[i];
        if (outputData[i] > maxProb)
        {
            maxProb = outputData[i];
            maxIdx = static_cast<int>(i);
        }
    }

    result.category = CONTENT_CATEGORIES[maxIdx];
    result.confidence = maxProb;

    return result;
}

#else

ContentClassifier::ContentClassifier(const std::string& modelPath) : modelPath_(modelPath), modelLoaded_(false)
{
    errorMessage_ = "ONNX Runtime not available. Please enable ENABLE_ONNX in CMake.";
}

ContentClassifier::~ContentClassifier() = default;

void ContentClassifier::loadModel() { modelLoaded_ = true; }

#endif

ClassificationResult ContentClassifier::classify(const std::vector<unsigned char>& imageData)
{
#if ENABLE_ONNX
    return runInference(imageData);
#else
    ClassificationResult result;
    result.category = "其他";
    result.confidence = 0.0f;
    result.allProbabilities = std::vector<float>(CONTENT_CATEGORIES.size(), 0.0f);
    result.modelName = "onnx";
    result.timestamp = get_current_timestamp();
    return result;
#endif
}

bool ContentClassifier::isModelLoaded() const
{
#if ENABLE_ONNX
    return modelLoaded_;
#else
    return false;
#endif
}

void ContentClassifier::unloadModel()
{
#if ENABLE_ONNX
    session_.reset();
    sessionOptions_.reset();
    env_.reset();
    modelLoaded_ = false;
#endif
}

std::string ContentClassifier::getModelType() const { return "onnx"; }