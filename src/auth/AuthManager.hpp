#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace auth
{

class AuthManager
{
public:
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;

    static AuthManager& get()
    {
        static AuthManager instance;
        return instance;
    }

    struct LoginResult
    {
        std::string access_token;
        std::string refresh_token;
        int expires_in;
        std::string username;
        std::string role;
    };

    struct RefreshResult
    {
        std::string access_token;
        int expires_in;
    };

    std::optional<LoginResult> login(const std::string& username, const std::string& password);
    std::optional<RefreshResult> refresh(const std::string& refresh_token);
    bool logout(const std::string& token);

    bool is_admin(const std::string& username);
    bool user_exists(const std::string& username);
    bool add_user(const std::string& username, const std::string& password_hash);
    bool remove_user(const std::string& username);

    std::string hash_password(const std::string& password);

private:
    AuthManager() = default;
};

} // namespace auth
