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
            FOREIGN KEY (owner) REFERENCES users(username)
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
    return true;
}

bool PhotoMetadataDb::insert_photo(const PhotoMetadata& photo)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        INSERT INTO photos (id, filename, filepath, size, owner, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?)
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

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool PhotoMetadataDb::update_photo(const PhotoMetadata& photo)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        UPDATE photos SET filename = ?, filepath = ?, size = ?, updated_at = ?
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
    sqlite3_bind_text(stmt, 5, photo.id.c_str(), -1, SQLITE_TRANSIENT);

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

    const char* sql = "SELECT id, filename, filepath, size, owner, created_at, updated_at FROM photos WHERE id = ?";
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
    const char* sql = "SELECT id, filename, filepath, size, owner, created_at, updated_at FROM photos WHERE owner = ? ORDER BY created_at DESC";
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
        results.push_back(photo);
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<PhotoMetadata> PhotoMetadataDb::get_all_photos()
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PhotoMetadata> results;
    const char* sql = "SELECT id, filename, filepath, size, owner, created_at, updated_at FROM photos ORDER BY created_at DESC";
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
