#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;

namespace db
{

struct PhotoMetadata
{
    std::string id;
    std::string filename;
    std::string filepath;
    int64_t size;
    std::string owner;
    std::string created_at;
    std::string updated_at;
};

class PhotoMetadataDb
{
public:
    static PhotoMetadataDb& get_instance();

    bool initialize(const std::string& db_path = "database/photo_metadata.db");

    bool insert_photo(const PhotoMetadata& photo);
    bool update_photo(const PhotoMetadata& photo);
    bool delete_photo(const std::string& id);
    std::optional<PhotoMetadata> get_photo_by_id(const std::string& id);
    std::vector<PhotoMetadata> get_photos_by_owner(const std::string& owner);
    std::vector<PhotoMetadata> get_all_photos();
    bool photo_exists(const std::string& id);

    void close();

private:
    PhotoMetadataDb() = default;
    ~PhotoMetadataDb();

    PhotoMetadataDb(const PhotoMetadataDb&) = delete;
    PhotoMetadataDb& operator=(const PhotoMetadataDb&) = delete;

    bool create_tables();

    static int photo_callback(void* data, int argc, char** argv, char** col_names);
    static int photo_list_callback(void* data, int argc, char** argv, char** col_names);
    static int photo_count_callback(void* data, int argc, char** argv, char** col_names);

    sqlite3* db_ = nullptr;
    std::mutex mutex_;
};

} // namespace db
