#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace auth
{

struct AuthContext
{
    std::string username;
    std::string role;
    bool is_authenticated;
};

class AuthInterceptor : public oatpp::web::server::interceptor::RequestInterceptor
{
public:
    AuthInterceptor();

    std::shared_ptr<oatpp::web::protocol::http::outgoing::Response>
    intercept(const std::shared_ptr<oatpp::web::protocol::http::incoming::Request>& request) override;

    static const std::string ATTRIBUTE_KEY;

private:
    static bool is_public_endpoint(const std::string& path);

    std::shared_ptr<oatpp::web::protocol::http::outgoing::Response> create_error_response(int status_code, const std::string& error,
                                                                                          const std::string& message);

    std::unordered_set<std::string> public_paths_;
    std::mutex mutex_;
};

} // namespace auth
