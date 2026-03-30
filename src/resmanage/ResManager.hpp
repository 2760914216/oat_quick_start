#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
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

    std::optional<bool> write_resource(ResourceType type, const std::string& filename, const std::vector<char>& data)
    {
        std::unique_lock lock(mutex_);
        auto it = typePathMap_.find(type);
        if (it == typePathMap_.end())
        {
            return std::nullopt;
        }
        fs::path fullPath = fs::path(it->second) / filename;
        std::ofstream file(fullPath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            return std::nullopt;
        }
        file.write(data.data(), data.size());
        return file.good();
    }

    std::optional<bool> write_resource(ResourceType type, const std::string& filename, const std::string& data)
    {
        std::unique_lock lock(mutex_);
        auto it = typePathMap_.find(type);
        if (it == typePathMap_.end())
        {
            return std::nullopt;
        }
        fs::path fullPath = fs::path(it->second) / filename;
        std::ofstream file(fullPath, std::ios::trunc);
        if (!file.is_open())
        {
            return std::nullopt;
        }
        file.write(data.data(), data.size());
        return file.good();
    }

    std::optional<bool> delete_resource(ResourceType type, const std::string& filename)
    {
        std::unique_lock lock(mutex_);
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
        return fs::remove(fullPath);
    }

    std::string get_temp_path() const
    {
        std::shared_lock lock(mutex_);
        return tempPath_;
    }

    bool create_temp_session(const std::string& session_id)
    {
        std::unique_lock lock(mutex_);
        fs::path sessionDir = fs::path(tempPath_) / session_id;
        if (fs::exists(sessionDir))
        {
            return false;
        }
        return fs::create_directories(sessionDir);
    }

    std::optional<bool> write_chunk(const std::string& session_id, int chunk_index, const std::vector<char>& data)
    {
        std::unique_lock lock(mutex_);
        fs::path chunkPath = fs::path(tempPath_) / session_id / ("chunk_" + std::to_string(chunk_index));
        std::ofstream file(chunkPath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            return std::nullopt;
        }
        file.write(data.data(), data.size());
        return file.good();
    }

    std::optional<bool> merge_chunks(const std::string& session_id, const std::string& output_path, int chunk_count)
    {
        std::unique_lock lock(mutex_);
        std::ofstream outFile(output_path, std::ios::binary | std::ios::trunc);
        if (!outFile.is_open())
        {
            return std::nullopt;
        }

        for (int i = 0; i < chunk_count; ++i)
        {
            fs::path chunkPath = fs::path(tempPath_) / session_id / ("chunk_" + std::to_string(i));
            if (!fs::exists(chunkPath))
            {
                return std::nullopt;
            }
            std::ifstream chunkFile(chunkPath, std::ios::binary);
            if (!chunkFile.is_open())
            {
                return std::nullopt;
            }
            outFile << chunkFile.rdbuf();
            chunkFile.close();
        }
        outFile.close();
        return fs::exists(output_path);
    }

    bool cleanup_session(const std::string& session_id)
    {
        std::unique_lock lock(mutex_);
        fs::path sessionDir = fs::path(tempPath_) / session_id;
        if (fs::exists(sessionDir))
        {
            return fs::remove_all(sessionDir) > 0;
        }
        return false;
    }

    void cleanup_expired_uploads(int max_age_hours = 24)
    {
        std::unique_lock lock(mutex_);
        if (!fs::exists(tempPath_))
        {
            return;
        }
        auto now = std::chrono::system_clock::now();
        for (const auto& entry : fs::directory_iterator(tempPath_))
        {
            if (!entry.is_directory())
            {
                continue;
            }
            auto mtime = fs::last_write_time(entry.path());
            auto age = now - std::chrono::system_clock::from_time_t(std::chrono::duration_cast<std::chrono::seconds>(mtime.time_since_epoch()).count());
            if (age > std::chrono::hours(max_age_hours))
            {
                fs::remove_all(entry.path());
            }
        }
    }

    void start_cleanup_thread(int interval_minutes = 60)
    {
        cleanupRunning_ = true;
        cleanupThread_ = std::thread(
            [this, interval_minutes]()
            {
                while (cleanupRunning_)
                {
                    std::this_thread::sleep_for(std::chrono::minutes(interval_minutes));
                    if (cleanupRunning_)
                    {
                        cleanup_expired_uploads();
                    }
                }
            });
    }

    void stop_cleanup_thread()
    {
        cleanupRunning_ = false;
        if (cleanupThread_.joinable())
        {
            cleanupThread_.join();
        }
    }

    int64_t get_file_size(ResourceType type, const std::string& filename)
    {
        std::shared_lock lock(mutex_);
        auto it = typePathMap_.find(type);
        if (it == typePathMap_.end())
        {
            return -1;
        }
        fs::path fullPath = fs::path(it->second) / filename;
        if (!fs::exists(fullPath))
        {
            return -1;
        }
        return static_cast<int64_t>(fs::file_size(fullPath));
    }

private:
    ResManager() : basePath_("res/"), tempPath_("res/temp/"), cleanupRunning_(false)
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
    std::string tempPath_;
    std::unordered_map<ResourceType, std::string> typePathMap_;
    mutable std::shared_mutex mutex_;
    std::atomic<bool> cleanupRunning_;
    std::thread cleanupThread_;
};

} // namespace res
