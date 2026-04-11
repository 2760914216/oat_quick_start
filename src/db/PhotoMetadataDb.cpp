#include "PhotoMetadataDb.hpp"
#include <cstring>
#include <filesystem>
#include <sqlite3.h>

namespace db
{

PhotoMetadataDb& PhotoMetadataDb::get_instance()
{
    static PhotoMetadataDb instance;
    return instance;
}

PhotoMetadataDb::~PhotoMetadataDb() { close(); }

bool PhotoMetadataDb::initialize(const std::string& db_path)
{
    std::lock_guard<std::mutex> lock(mutex_);

    namespace fs = std::filesystem;
    fs::path dbDir = fs::path(db_path).parent_path();
    if (!dbDir.empty() && !fs::exists(dbDir))
    {
        if (!fs::create_directories(dbDir))
        {
            return false;
        }
    }

    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK)
    {
        return false;
    }

    return create_tables();
}

bool PhotoMetadataDb::create_tables()
{
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS photos (
            id TEXT PRIMARY KEY,
            filename TEXT NOT NULL,
            filepath TEXT NOT NULL,
            size INTEGER DEFAULT 0,
            owner TEXT NOT NULL,
            created_at TEXT NOT NULL,
            updated_at TEXT,
            scene_category TEXT,
            content_category TEXT,
            classification_status TEXT DEFAULT 'pending',
            classification_dirty INTEGER DEFAULT 0,
            classified_at TEXT,
            schema_version TEXT DEFAULT '1.1'
        );
        CREATE INDEX IF NOT EXISTS idx_photos_owner ON photos(owner);
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        if (errMsg)
        {
            sqlite3_free(errMsg);
        }
        return false;
    }

    const char* alter_sql = R"(
        ALTER TABLE photos ADD COLUMN scene_category TEXT;
        ALTER TABLE photos ADD COLUMN content_category TEXT;
        ALTER TABLE photos ADD COLUMN classification_status TEXT DEFAULT 'pending';
        ALTER TABLE photos ADD COLUMN classification_dirty INTEGER DEFAULT 0;
        ALTER TABLE photos ADD COLUMN classified_at TEXT;
        ALTER TABLE photos ADD COLUMN schema_version TEXT DEFAULT '1.1';
    )";
    rc = sqlite3_exec(db_, alter_sql, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
    }

    const char* edits_sql = R"(
        CREATE TABLE IF NOT EXISTS photo_edits (
            id TEXT PRIMARY KEY,
            photo_id TEXT NOT NULL,
            edit_params TEXT,
            created_at TEXT NOT NULL,
            updated_at TEXT,
            FOREIGN KEY (photo_id) REFERENCES photos(id) ON DELETE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_photo_edits_photo_id ON photo_edits(photo_id);
    )";
    rc = sqlite3_exec(db_, edits_sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        if (errMsg)
        {
            sqlite3_free(errMsg);
        }
        return false;
    }

    const char* thumbnails_sql = R"(
        CREATE TABLE IF NOT EXISTS photo_thumbnails (
            id TEXT PRIMARY KEY,
            photo_id TEXT NOT NULL,
            thumbnail_path TEXT NOT NULL,
            size_type TEXT NOT NULL,
            created_at TEXT NOT NULL,
            FOREIGN KEY (photo_id) REFERENCES photos(id) ON DELETE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_photo_thumbnails_photo_id_size ON photo_thumbnails(photo_id, size_type);
    )";
    rc = sqlite3_exec(db_, thumbnails_sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        if (errMsg)
        {
            sqlite3_free(errMsg);
        }
        return false;
    }

    const char* models_sql = R"(
        CREATE TABLE IF NOT EXISTS classification_models (
            id TEXT PRIMARY KEY,
            model_name TEXT NOT NULL,
            model_path TEXT NOT NULL,
            model_type TEXT NOT NULL,
            is_active INTEGER DEFAULT 0,
            created_at TEXT NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_classification_models_type_active ON classification_models(model_type, is_active);
    )";
    rc = sqlite3_exec(db_, models_sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        if (errMsg)
        {
            sqlite3_free(errMsg);
        }
        return false;
    }

    return true;
}

bool PhotoMetadataDb::insert_photo(const PhotoMetadata& photo)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        INSERT INTO photos (id, filename, filepath, size, owner, created_at, updated_at, scene_category, content_category, classification_status, classification_dirty, classified_at, schema_version)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, photo.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, photo.filename.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, photo.filepath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, photo.size);
    sqlite3_bind_text(stmt, 5, photo.owner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, photo.created_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, photo.updated_at.empty() ? nullptr : photo.updated_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, photo.scene_category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, photo.content_category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, photo.classification_status.empty() ? "pending" : photo.classification_status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 11, photo.classification_dirty);
    sqlite3_bind_text(stmt, 12, photo.classified_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, photo.schema_version.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool PhotoMetadataDb::update_photo(const PhotoMetadata& photo)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        UPDATE photos SET filename = ?, filepath = ?, size = ?, updated_at = ?, scene_category = ?, content_category = ?, classification_status = ?, classification_dirty = ?, classified_at = ?, schema_version = ?
        WHERE id = ?
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, photo.filename.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, photo.filepath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, photo.size);
    sqlite3_bind_text(stmt, 4, photo.updated_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, photo.scene_category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, photo.content_category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, photo.classification_status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, photo.classification_dirty);
    sqlite3_bind_text(stmt, 9, photo.classified_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, photo.schema_version.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, photo.id.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool PhotoMetadataDb::delete_photo(const std::string& id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "DELETE FROM photos WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::optional<PhotoMetadata> PhotoMetadataDb::get_photo_by_id(const std::string& id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id, filename, filepath, size, owner, created_at, updated_at, scene_category, content_category, classification_status, "
                      "classification_dirty, classified_at, schema_version FROM photos WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;

    PhotoMetadata result;
    bool found = false;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        found = true;
        result.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) : "";
        result.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) : "";
        result.filepath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) : "";
        result.size = sqlite3_column_int64(stmt, 3);
        result.owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) : "";
        result.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) : "";
        result.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)) : "";
        result.scene_category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)) : "";
        result.content_category =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)) : "";
        result.classification_status =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)) : "";
        result.classification_dirty = sqlite3_column_int(stmt, 10);
        result.classified_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11)) : "";
        result.schema_version =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12)) : "";
    }

    sqlite3_finalize(stmt);

    if (found)
    {
        return result;
    }
    return std::nullopt;
}

std::vector<PhotoMetadata> PhotoMetadataDb::get_photos_by_owner(const std::string& owner)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PhotoMetadata> results;
    const char* sql = "SELECT id, filename, filepath, size, owner, created_at, updated_at, scene_category, content_category, classification_status, "
                      "classification_dirty, classified_at, schema_version FROM photos WHERE owner = ? ORDER "
                      "BY created_at DESC";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return results;
    }

    sqlite3_bind_text(stmt, 1, owner.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        PhotoMetadata photo;
        photo.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) : "";
        photo.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) : "";
        photo.filepath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) : "";
        photo.size = sqlite3_column_int64(stmt, 3);
        photo.owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) : "";
        photo.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) : "";
        photo.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)) : "";
        photo.scene_category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)) : "";
        photo.content_category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)) : "";
        photo.classification_status =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)) : "";
        photo.classification_dirty = sqlite3_column_int(stmt, 10);
        photo.classified_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11)) : "";
        photo.schema_version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12)) : "";
        results.push_back(photo);
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<PhotoMetadata> PhotoMetadataDb::get_all_photos()
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PhotoMetadata> results;
    const char* sql = "SELECT id, filename, filepath, size, owner, created_at, updated_at, scene_category, content_category, classification_status, "
                      "classification_dirty, classified_at, schema_version FROM photos ORDER BY created_at DESC";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return results;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        PhotoMetadata photo;
        photo.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) : "";
        photo.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) : "";
        photo.filepath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) : "";
        photo.size = sqlite3_column_int64(stmt, 3);
        photo.owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) : "";
        photo.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) : "";
        photo.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)) : "";
        photo.scene_category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)) : "";
        photo.content_category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)) : "";
        photo.classification_status =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)) : "";
        photo.classification_dirty = sqlite3_column_int(stmt, 10);
        photo.classified_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11)) : "";
        photo.schema_version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12)) : "";
        results.push_back(photo);
    }

    sqlite3_finalize(stmt);
    return results;
}

bool PhotoMetadataDb::photo_exists(const std::string& id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT COUNT(*) FROM photos WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }

    sqlite3_finalize(stmt);
    return exists;
}

bool PhotoMetadataDb::insert_edit(const PhotoEdit& edit)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        INSERT INTO photo_edits (id, photo_id, edit_params, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, edit.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, edit.photo_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, edit.edit_params.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, edit.created_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, edit.updated_at.empty() ? nullptr : edit.updated_at.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool PhotoMetadataDb::update_edit(const PhotoEdit& edit)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        UPDATE photo_edits SET edit_params = ?, updated_at = ?
        WHERE photo_id = ?
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, edit.edit_params.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, edit.updated_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, edit.photo_id.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::optional<PhotoEdit> PhotoMetadataDb::get_edit_by_photo_id(const std::string& photo_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id, photo_id, edit_params, created_at, updated_at FROM photo_edits WHERE photo_id = ?";
    sqlite3_stmt* stmt = nullptr;

    PhotoEdit result;
    bool found = false;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, photo_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        found = true;
        result.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) : "";
        result.photo_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) : "";
        result.edit_params = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) : "";
        result.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) : "";
        result.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) : "";
    }

    sqlite3_finalize(stmt);

    if (found)
    {
        return result;
    }
    return std::nullopt;
}

bool PhotoMetadataDb::delete_edit(const std::string& photo_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "DELETE FROM photo_edits WHERE photo_id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, photo_id.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool PhotoMetadataDb::insert_thumbnail(const PhotoThumbnail& thumb)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        INSERT INTO photo_thumbnails (id, photo_id, thumbnail_path, size_type, created_at)
        VALUES (?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, thumb.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, thumb.photo_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, thumb.thumbnail_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, thumb.size_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, thumb.created_at.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<PhotoThumbnail> PhotoMetadataDb::get_thumbnails_by_photo_id(const std::string& photo_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PhotoThumbnail> results;
    const char* sql = "SELECT id, photo_id, thumbnail_path, size_type, created_at FROM photo_thumbnails WHERE photo_id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return results;
    }

    sqlite3_bind_text(stmt, 1, photo_id.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        PhotoThumbnail thumb;
        thumb.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) : "";
        thumb.photo_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) : "";
        thumb.thumbnail_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) : "";
        thumb.size_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) : "";
        thumb.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) : "";
        results.push_back(thumb);
    }

    sqlite3_finalize(stmt);
    return results;
}

bool PhotoMetadataDb::delete_thumbnails_by_photo_id(const std::string& photo_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "DELETE FROM photo_thumbnails WHERE photo_id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, photo_id.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

int PhotoMetadataDb::photo_edit_callback(void* data, int argc, char** argv, char** col_names) { return 0; }

int PhotoMetadataDb::photo_thumbnail_callback(void* data, int argc, char** argv, char** col_names) { return 0; }

int PhotoMetadataDb::classification_model_callback(void* data, int argc, char** argv, char** col_names) { return 0; }

bool PhotoMetadataDb::insert_model(const ClassificationModel& model)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        INSERT INTO classification_models (id, model_name, model_path, model_type, is_active, created_at)
        VALUES (?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, model.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, model.model_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, model.model_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, model.model_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, model.is_active);
    sqlite3_bind_text(stmt, 6, model.created_at.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::optional<ClassificationModel> PhotoMetadataDb::get_model_by_type(const std::string& model_type)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id, model_name, model_path, model_type, is_active, created_at FROM classification_models WHERE model_type = ?";
    sqlite3_stmt* stmt = nullptr;

    ClassificationModel result;
    bool found = false;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, model_type.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        found = true;
        result.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) : "";
        result.model_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) : "";
        result.model_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) : "";
        result.model_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) : "";
        result.is_active = sqlite3_column_int(stmt, 4);
        result.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) : "";
    }

    sqlite3_finalize(stmt);

    if (found)
    {
        return result;
    }
    return std::nullopt;
}

std::vector<ClassificationModel> PhotoMetadataDb::get_active_models()
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ClassificationModel> results;
    const char* sql = "SELECT id, model_name, model_path, model_type, is_active, created_at FROM classification_models WHERE is_active = 1";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return results;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ClassificationModel model;
        model.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) : "";
        model.model_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) : "";
        model.model_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) : "";
        model.model_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) : "";
        model.is_active = sqlite3_column_int(stmt, 4);
        model.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) : "";
        results.push_back(model);
    }

    sqlite3_finalize(stmt);
    return results;
}

bool PhotoMetadataDb::update_model_active(const std::string& id, bool is_active)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "UPDATE classification_models SET is_active = ? WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, is_active ? 1 : 0);
    sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool PhotoMetadataDb::delete_model(const std::string& id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "DELETE FROM classification_models WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

void PhotoMetadataDb::close()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_)
    {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

} // namespace db
