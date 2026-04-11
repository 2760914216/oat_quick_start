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
    std::string scene_category;
    std::string content_category;
    std::string classification_status; // pending, processing, completed, failed
    int classification_dirty;          // 0 = clean, 1 = dirty
    std::string classified_at;         // ISO timestamp when classification was completed
    std::string schema_version;        // e.g., "1.0", "1.1"
};

struct PhotoEdit
{
    std::string id;
    std::string photo_id;
    std::string edit_params; // JSON string
    std::string created_at;
    std::string updated_at;
};

struct PhotoThumbnail
{
    std::string id;
    std::string photo_id;
    std::string thumbnail_path;
    std::string size_type; // 'list' (150x150) or 'detail' (400x400)
    std::string created_at;
};

struct ClassificationModel
{
    std::string id;
    std::string model_name;
    std::string model_path;
    std::string model_type; // 'scene' or 'content'
    int is_active;          // 0 = inactive, 1 = active
    std::string created_at;
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

    bool insert_edit(const PhotoEdit& edit);
    bool update_edit(const PhotoEdit& edit);
    std::optional<PhotoEdit> get_edit_by_photo_id(const std::string& photo_id);
    bool delete_edit(const std::string& photo_id);

    bool insert_thumbnail(const PhotoThumbnail& thumb);
    std::vector<PhotoThumbnail> get_thumbnails_by_photo_id(const std::string& photo_id);
    bool delete_thumbnails_by_photo_id(const std::string& photo_id);

    bool insert_model(const ClassificationModel& model);
    std::optional<ClassificationModel> get_model_by_type(const std::string& model_type);
    std::vector<ClassificationModel> get_active_models();
    bool update_model_active(const std::string& id, bool is_active);
    bool delete_model(const std::string& id);

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
    static int photo_edit_callback(void* data, int argc, char** argv, char** col_names);
    static int photo_thumbnail_callback(void* data, int argc, char** argv, char** col_names);
    static int classification_model_callback(void* data, int argc, char** argv, char** col_names);

    sqlite3* db_ = nullptr;
    std::mutex mutex_;
};

} // namespace db
