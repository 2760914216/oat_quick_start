#pragma once

#include "auth/AuthManager.hpp"
#include "auth/JwtHelper.hpp"
#include "config/ConfigManager.hpp"
#include "dto/AuthDTOs.hpp"
#include "dto/DTOs.hpp"

#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController)

class AuthController : public oatpp::web::server::api::ApiController
{
public:
    AuthController(OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper)) : oatpp::web::server::api::ApiController(objectMapper) {}

public:
    ENDPOINT("POST", "/api/auth/login", login, BODY_DTO(Object<LoginRequestDto>, request))
    {
        auto username = request->username;
        auto password = request->password;

        if (!username || !password)
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "invalid_request";
            error->message = "Username and password are required";
            return createDtoResponse(Status::CODE_400, error);
        }

        auto& authManager = auth::AuthManager::get();
        auto result = authManager.login(username->c_str(), password->c_str());

        if (!result)
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "invalid_credentials";
            error->message = "Invalid username or password";
            return createDtoResponse(Status::CODE_401, error);
        }

        auto response = LoginResponseDto::createShared();
        response->access_token = result->access_token.c_str();
        response->refresh_token = result->refresh_token.c_str();
        response->expires_in = result->expires_in;
        response->username = result->username.c_str();
        response->role = result->role.c_str();

        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("POST", "/api/auth/refresh", refresh, BODY_DTO(Object<RefreshRequestDto>, request))
    {
        auto refreshToken = request->refresh_token;

        if (!refreshToken)
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "invalid_request";
            error->message = "Refresh token is required";
            return createDtoResponse(Status::CODE_400, error);
        }

        auto& authManager = auth::AuthManager::get();
        auto result = authManager.refresh(refreshToken->c_str());

        if (!result)
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "invalid_token";
            error->message = "Invalid or expired refresh token";
            return createDtoResponse(Status::CODE_401, error);
        }

        auto response = RefreshResponseDto::createShared();
        response->access_token = result->access_token.c_str();
        response->expires_in = result->expires_in;

        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("POST", "/api/auth/logout", logout, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request))
    {
        auto authHeader = request->getHeader("Authorization");
        if (!authHeader)
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "unauthorized";
            error->message = "Authorization header required";
            return createDtoResponse(Status::CODE_401, error);
        }

        auto headerStr = std::string(authHeader->c_str());
        std::string prefix = "Bearer ";
        if (headerStr.substr(0, prefix.length()) != prefix)
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "invalid_token";
            error->message = "Invalid authorization header format";
            return createDtoResponse(Status::CODE_401, error);
        }

        auto token = headerStr.substr(prefix.length());
        auto& authManager = auth::AuthManager::get();

        if (!authManager.logout(token))
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "invalid_token";
            error->message = "Invalid token";
            return createDtoResponse(Status::CODE_401, error);
        }

        auto response = LogoutResponseDto::createShared();
        response->message = "Logged out successfully";
        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("GET", "/health", health)
    {
        auto dto = MessageDto::createShared();
        dto->statusCode = 200;
        dto->message = "OK";
        return createDtoResponse(Status::CODE_200, dto);
    }

private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);
};

#include OATPP_CODEGEN_END(ApiController)
