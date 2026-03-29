#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace res
{
namespace fs = std::filesystem;

class ResManager
{

public:
    enum class ResourceType
    {
        Texture = 1,
        Model,
        Audio,
        Config,
        Shaders,
        Font,
        Video,
        Photo,
    };

    struct ResourceInfo
    {
        std::string path;
        bool exists;
    };

public:
    ResManager(const ResManager&) = delete;
    ResManager& operator=(const ResManager&) = delete;

    static ResManager& get_instance()
    {
        static ResManager instance;
        return instance;
    }

    std::string get_base_path() const
    {
        std::shared_lock lock(mutex_);
        return basePath_;
    }

    std::string get_res_path(ResourceType type)
    {
        std::shared_lock lock(mutex_);
        auto it = typePathMap_.find(type);
        if (it != typePathMap_.end())
        {
            return it->second;
        }
        return "";
    }

    bool resource_exists(ResourceType type, const std::string& filename)
    {
        std::shared_lock lock(mutex_);
        auto it = typePathMap_.find(type);
        if (it == typePathMap_.end())
        {
            return false;
        }
        fs::path fullPath = fs::path(it->second) / filename;
        return fs::exists(fullPath);
    }

    std::optional<std::string> get_resource_path(ResourceType type, const std::string& filename)
    {
        std::shared_lock lock(mutex_);
        auto it = typePathMap_.find(type);
        if (it == typePathMap_.end())
        {
            return std::nullopt;
        }
        fs::path fullPath = fs::path(it->second) / filename;
        if (!fs::exists(fullPath))
        {
            return std::nullopt;
        }
        return fullPath.string();
    }

    std::optional<std::vector<char>> read_resource(ResourceType type, const std::string& filename)
    {
        auto fullPath = get_resource_path(type, filename);
        if (!fullPath)
        {
            return std::nullopt;
        }

        std::ifstream file(*fullPath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return std::nullopt;
        }

        auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(static_cast<size_t>(size));
        if (!file.read(buffer.data(), size))
        {
            return std::nullopt;
        }
        return buffer;
    }

    std::optional<std::string> read_resource_text(ResourceType type, const std::string& filename)
    {
        auto fullPath = get_resource_path(type, filename);
        if (!fullPath)
        {
            return std::nullopt;
        }

        std::ifstream file(*fullPath);
        if (!file.is_open())
        {
            return std::nullopt;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return content;
    }

    std::vector<std::string> list_resources(ResourceType type)
    {
        std::shared_lock lock(mutex_);
        std::vector<std::string> files;
        auto it = typePathMap_.find(type);
        if (it == typePathMap_.end())
        {
            return files;
        }

        if (!fs::exists(it->second))
        {
            return files;
        }

        for (const auto& entry : fs::directory_iterator(it->second))
        {
            if (entry.is_regular_file())
            {
                files.push_back(entry.path().filename().string());
            }
        }
        return files;
    }

    void refresh()
    {
        std::unique_lock lock(mutex_);
        init_directories();
        build_type_path_map();
    }

    std::string generate_photo_id()
    {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        std::string id = std::to_string(timestamp) + "_" + std::to_string(rand() % 10000);
        return id;
    }

private:
    ResManager() : basePath_("res/")
    {
        std::unique_lock lock(mutex_);
        init_directories();
        build_type_path_map();
    }

    void init_directories()
    {
        if (!fs::exists(basePath_))
        {
            fs::create_directories(basePath_);
        }

        for (const auto& type : typeNames_)
        {
            fs::path dir = fs::path(basePath_) / type.second;
            if (!fs::exists(dir))
            {
                fs::create_directories(dir);
            }
        }
    }

    void build_type_path_map()
    {
        typePathMap_.clear();
        for (const auto& type : typeNames_)
        {
            fs::path fullPath = fs::path(basePath_) / type.second;
            typePathMap_[type.first] = normalize_path(fullPath.string());
        }
    }

    std::string normalize_path(const std::string& path)
    {
        std::string ret = path;
        if (!ret.empty() && ret.back() != '/')
        {
            ret += '/';
        }
        return ret;
    }

    static inline const std::unordered_map<ResourceType, std::string> typeNames_ = {
        {ResourceType::Texture, "Texture"}, {ResourceType::Model, "Model"}, {ResourceType::Audio, "Audio"}, {ResourceType::Config, "Config"},
        {ResourceType::Shaders, "Shaders"}, {ResourceType::Font, "Font"},   {ResourceType::Video, "Video"}, {ResourceType::Photo, "Photo"},
    };

    std::string basePath_;
    std::unordered_map<ResourceType, std::string> typePathMap_;
    mutable std::shared_mutex mutex_;
};

} // namespace res
