#pragma once

#include <ctime>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

namespace config
{

struct ServerConfig
{
    std::string host = "0.0.0.0";
    int port = 1145;
};

struct JwtConfig
{
    std::string secret = "default-secret-change-in-production-min-32-chars";
    int access_token_expire = 3600;
    int refresh_token_expire = 604800;
};

struct UserInfo
{
    std::string username;
    std::string password_hash;
    std::string created_at;
};

struct AdminConfig
{
    std::string username;
    std::string password_hash;
};

class ConfigManager
{
public:
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    static ConfigManager& get()
    {
        static ConfigManager instance;
        return instance;
    }

    bool load(const std::string& path);
    bool save();

    std::string get_jwt_secret() const;
    int get_access_token_expire() const;
    int get_refresh_token_expire() const;
    int get_server_port() const;
    std::string get_server_host() const;

    bool validate_user(const std::string& username, const std::string& password_hash);
    bool validate_admin(const std::string& username, const std::string& password_hash);
    bool is_admin(const std::string& username) const;

    std::vector<UserInfo> get_all_users() const;
    bool add_user(const std::string& username, const std::string& password_hash);
    bool remove_user(const std::string& username);
    bool user_exists(const std::string& username) const;

    std::string get_config_path() const { return configPath_; }

private:
    ConfigManager() = default;

    std::string configPath_;
    ServerConfig serverConfig_;
    JwtConfig jwtConfig_;
    AdminConfig adminConfig_;
    std::vector<UserInfo> users_;

    mutable std::shared_mutex mutex_;
};

} // namespace config
