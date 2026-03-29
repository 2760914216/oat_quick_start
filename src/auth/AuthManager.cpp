#include "AuthManager.hpp"
#include "../config/ConfigManager.hpp"
#include "JwtHelper.hpp"
#include "TokenStore.hpp"
#include <cstring>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

namespace
{

std::string bcrypt_simple_hash(const std::string& password)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.length(), hash);

    char hex_hash[65];
    for (int i = 0; i < 32; i++)
    {
        sprintf(hex_hash + i * 2, "%02x", hash[i]);
    }
    hex_hash[64] = '\0';

    return std::string(hex_hash);
}

} // namespace

namespace auth
{

std::optional<AuthManager::LoginResult> AuthManager::login(const std::string& username, const std::string& password)
{
    auto& config = config::ConfigManager::get();
    auto& jwt = JwtHelper::get();

    std::string password_hash = hash_password(password);

    if (!config.validate_user(username, password_hash))
    {
        return std::nullopt;
    }

    LoginResult result;
    result.username = username;
    result.role = config.is_admin(username) ? "admin" : "user";
    result.expires_in = config.get_access_token_expire();

    result.access_token = jwt.create_token(username, result.role, result.expires_in);
    result.refresh_token = jwt.create_token(username, result.role, config.get_refresh_token_expire());

    if (result.access_token.empty() || result.refresh_token.empty())
    {
        return std::nullopt;
    }

    return result;
}

std::optional<AuthManager::RefreshResult> AuthManager::refresh(const std::string& refresh_token)
{
    auto& jwt = JwtHelper::get();
    auto& tokenStore = TokenStore::get();

    if (tokenStore.is_revoked(refresh_token))
    {
        return std::nullopt;
    }

    auto payload = jwt.verify_token(refresh_token);
    if (!payload || payload->role.empty())
    {
        return std::nullopt;
    }

    auto& config = config::ConfigManager::get();
    int expires_in = config.get_access_token_expire();

    RefreshResult result;
    result.access_token = jwt.create_token(payload->username, payload->role, expires_in);
    result.expires_in = expires_in;

    if (result.access_token.empty())
    {
        return std::nullopt;
    }

    return result;
}

bool AuthManager::logout(const std::string& token)
{
    auto& tokenStore = TokenStore::get();
    auto& jwt = JwtHelper::get();

    auto payload = jwt.verify_token(token);
    if (!payload)
    {
        return false;
    }

    tokenStore.revoke(token);
    return true;
}

bool AuthManager::is_admin(const std::string& username) { return config::ConfigManager::get().is_admin(username); }

bool AuthManager::user_exists(const std::string& username) { return config::ConfigManager::get().user_exists(username); }

bool AuthManager::add_user(const std::string& username, const std::string& password_hash)
{
    return config::ConfigManager::get().add_user(username, password_hash);
}

bool AuthManager::remove_user(const std::string& username) { return config::ConfigManager::get().remove_user(username); }

std::string AuthManager::hash_password(const std::string& password) { return bcrypt_simple_hash(password); }

} // namespace auth
