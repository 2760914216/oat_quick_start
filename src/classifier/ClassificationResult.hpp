#ifndef CLASSIFICATION_RESULT_HPP
#define CLASSIFICATION_RESULT_HPP

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief Structure holding classification result
 */
struct ClassificationResult
{
    std::string category;                ///< Predicted class name
    float confidence;                    ///< 0.0 - 1.0
    std::vector<float> allProbabilities; ///< All class probabilities
    std::string modelName;               ///< Name of model used
    std::string timestamp;               ///< ISO timestamp

    /**
     * @brief Default constructor
     */
    ClassificationResult() : confidence(0.0f) {}

    /**
     * @brief Convert to JSON string
     * @return JSON formatted string
     */
    std::string toJson() const
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4);
        oss << "{";
        oss << "\"category\":\"" << category << "\",";
        oss << "\"confidence\":" << confidence << ",";
        oss << "\"allProbabilities\":[";
        for (size_t i = 0; i < allProbabilities.size(); ++i)
        {
            if (i > 0) oss << ",";
            oss << allProbabilities[i];
        }
        oss << "],";
        oss << "\"modelName\":\"" << modelName << "\",";
        oss << "\"timestamp\":\"" << timestamp << "\"";
        oss << "}";
        return oss.str();
    }
};

#endif // CLASSIFICATION_RESULT_HPP