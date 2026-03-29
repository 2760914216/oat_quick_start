#pragma once

#include <ctime>
#include <optional>
#include <string>

namespace auth
{

struct TokenPayload
{
    std::string username;
    std::string role;
    int64_t iat;
    int64_t exp;
};

class JwtHelper
{
public:
    JwtHelper(const JwtHelper&) = delete;
    JwtHelper& operator=(const JwtHelper&) = delete;

    static JwtHelper& get()
    {
        static JwtHelper instance;
        return instance;
    }

    void set_secret(const std::string& secret);
    std::string create_token(const std::string& username, const std::string& role, int expire_seconds);
    std::optional<TokenPayload> verify_token(const std::string& token);

private:
    JwtHelper() = default;

    std::string secret_;
};

} // namespace auth
