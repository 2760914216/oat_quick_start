#include "AuthInterceptor.hpp"
#include "../auth/JwtHelper.hpp"
#include "../auth/TokenStore.hpp"
#include "oatpp/core/data/resource/Resource.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"

namespace auth
{

const std::string AuthInterceptor::ATTRIBUTE_KEY = "auth_context";

AuthInterceptor::AuthInterceptor() { public_paths_ = {"/health", "/api/auth/login", "/api/auth/refresh"}; }

std::shared_ptr<oatpp::web::protocol::http::outgoing::Response>
AuthInterceptor::intercept(const std::shared_ptr<oatpp::web::protocol::http::incoming::Request>& request)
{
    auto path = request->getPath();
    std::string pathStr = path ? path->std_str() : "";

    if (public_paths_.find(pathStr) != public_paths_.end())
    {
        return nullptr;
    }

    auto authHeader = request->getHeader("Authorization");
    if (!authHeader)
    {
        auto response = oatpp::web::protocol::http::outgoing::Response::createShared(oatpp::web::protocol::http::Status::CODE_401, nullptr);
        response->putHeader("Content-Type", "application/json");
        auto body = oatpp::String("{\"error\":\"unauthorized\",\"message\":\"Authorization header required\"}");
        response->setBody(oatpp::web::protocol::http::outgoing::FixedBodyResource::createShared(body));
        return response;
    }

    auto headerStr = authHeader->std_str();
    std::string prefix = "Bearer ";
    if (headerStr.substr(0, prefix.length()) != prefix)
    {
        auto response = oatpp::web::protocol::http::outgoing::Response::createShared(oatpp::web::protocol::http::Status::CODE_401, nullptr);
        response->putHeader("Content-Type", "application/json");
        auto body = oatpp::String("{\"error\":\"invalid_token\",\"message\":\"Invalid authorization header format\"}");
        response->setBody(oatpp::web::protocol::http::outgoing::FixedBodyResource::createShared(body));
        return response;
    }

    auto token = headerStr.substr(prefix.length());
    auto& jwt = JwtHelper::get();
    auto& tokenStore = TokenStore::get();

    if (tokenStore.is_revoked(token))
    {
        auto response = oatpp::web::protocol::http::outgoing::Response::createShared(oatpp::web::protocol::http::Status::CODE_401, nullptr);
        response->putHeader("Content-Type", "application/json");
        auto body = oatpp::String("{\"error\":\"token_revoked\",\"message\":\"Token has been revoked\"}");
        response->setBody(oatpp::web::protocol::http::outgoing::FixedBodyResource::createShared(body));
        return response;
    }

    auto payload = jwt.verify_token(token);
    if (!payload)
    {
        auto response = oatpp::web::protocol::http::outgoing::Response::createShared(oatpp::web::protocol::http::Status::CODE_401, nullptr);
        response->putHeader("Content-Type", "application/json");
        auto body = oatpp::String("{\"error\":\"invalid_token\",\"message\":\"Invalid or expired token\"}");
        response->setBody(oatpp::web::protocol::http::outgoing::FixedBodyResource::createShared(body));
        return response;
    }

    auto context = std::make_shared<AuthContext>();
    context->username = payload->username;
    context->role = payload->role;
    context->is_authenticated = true;

    request->setAttribute(ATTRIBUTE_KEY, context);

    return nullptr;
}

} // namespace auth
