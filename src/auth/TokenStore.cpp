#include "TokenStore.hpp"
#include <chrono>

namespace auth
{

TokenStore::TokenStore() : running_(true)
{
    cleanupThread_ = std::thread(
        [this]()
        {
            while (running_)
            {
                std::this_thread::sleep_for(std::chrono::minutes(1));
                cleanup_expired();
            }
        });
}

TokenStore::~TokenStore()
{
    running_ = false;
    if (cleanupThread_.joinable())
    {
        cleanupThread_.join();
    }
}

void TokenStore::revoke(const std::string& token)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t now = std::time(nullptr);
    revokedTokens_[token] = {now + 604800};
}

bool TokenStore::is_revoked(const std::string& token)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = revokedTokens_.find(token);
    if (it == revokedTokens_.end())
    {
        return false;
    }
    int64_t now = std::time(nullptr);
    if (it->second.expiry_time < now)
    {
        revokedTokens_.erase(it);
        return false;
    }
    return true;
}

void TokenStore::cleanup_expired()
{
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t now = std::time(nullptr);
    for (auto it = revokedTokens_.begin(); it != revokedTokens_.end();)
    {
        if (it->second.expiry_time < now)
        {
            it = revokedTokens_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace auth
