#pragma once

#include <ctime>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace auth
{

class TokenStore
{
public:
    TokenStore(const TokenStore&) = delete;
    TokenStore& operator=(const TokenStore&) = delete;

    static TokenStore& get()
    {
        static TokenStore instance;
        return instance;
    }

    void revoke(const std::string& token);
    bool is_revoked(const std::string& token);
    void cleanup_expired();

private:
    TokenStore();
    ~TokenStore();

    struct RevokedToken
    {
        int64_t expiry_time;
    };

    std::unordered_map<std::string, RevokedToken> revokedTokens_;
    std::mutex mutex_;
    std::thread cleanupThread_;
    bool running_;
};

} // namespace auth
