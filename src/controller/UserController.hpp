#pragma once

#include "auth/AuthManager.hpp"
#include "auth/JwtHelper.hpp"
#include "config/ConfigManager.hpp"
#include "dto/AuthDTOs.hpp"

#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include <oatpp/Environment.hpp>
#include <oatpp/base/Log.hpp>

#include OATPP_CODEGEN_BEGIN(ApiController)

class UserController : public oatpp::web::server::api::ApiController
{
public:
    UserController(OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper)) : oatpp::web::server::api::ApiController(objectMapper) {}

public:
    ENDPOINT("GET", "/api/users", listUsers, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request))
    {
        auto authHeader = request->getHeader("Authorization");
        auto headerStr = std::string(authHeader->c_str());
        auto token = headerStr.substr(7);

        auto& jwt = auth::JwtHelper::get();
        auto payload = jwt.verify_token(token);

        if (!payload || payload->role != "admin")
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "forbidden";
            error->message = "Admin access required";
            return createDtoResponse(Status::CODE_403, error);
        }

        auto& config = config::ConfigManager::get();
        auto users = config.get_all_users();

        auto response = UserListDto::createShared();
        response->users = oatpp::Vector<Object<UserDto>>::createShared();

        for (const auto& user : users)
        {
            auto userDto = UserDto::createShared();
            userDto->username = user.username.c_str();
            userDto->created_at = user.created_at.c_str();
            response->users->push_back(userDto);
        }

        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("POST", "/api/users", createUser, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
             BODY_DTO(Object<CreateUserRequestDto>, body))
    {
        OATPP_LOGd("ENDPOINT post /api/users", "ready to create a new user");
        auto authHeader = request->getHeader("Authorization");
        auto headerStr = std::string(authHeader->c_str());
        auto token = headerStr.substr(7);

        auto& jwt = auth::JwtHelper::get();
        auto payload = jwt.verify_token(token);

        if (!payload || payload->role != "admin")
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "forbidden";
            error->message = "Admin access required";
            return createDtoResponse(Status::CODE_403, error);
        }
        OATPP_LOGd("ENDPOINT post /api/users", "admin verified");
        auto username = body->username;
        auto password = body->password;

        if (!username || !password)
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "invalid_request";
            error->message = "Username and password are required";
            return createDtoResponse(Status::CODE_400, error);
        }
        OATPP_LOGd("ENDPOINT post /api/users", "user profile valid");

        auto& authManager = auth::AuthManager::get();

        if (authManager.user_exists(username->c_str()))
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "user_exists";
            error->message = "User already exists";
            return createDtoResponse(Status::CODE_409, error);
        }
        OATPP_LOGd("ENDPOINT post /api/users", "user profile conflict");

        auto passwordHash = authManager.hash_password(password->c_str());

        if (!authManager.add_user(username->c_str(), passwordHash))
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "create_failed";
            error->message = "Failed to create user";
            return createDtoResponse(Status::CODE_500, error);
        }

        auto response = UserDto::createShared();
        response->username = username;

        return createDtoResponse(Status::CODE_201, response);
    }

    ENDPOINT("DELETE", "/api/users/{username}", deleteUser, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
             PATH(String, username))
    {
        auto authHeader = request->getHeader("Authorization");
        auto headerStr = std::string(authHeader->c_str());
        auto token = headerStr.substr(7);

        auto& jwt = auth::JwtHelper::get();
        auto payload = jwt.verify_token(token);

        if (!payload || payload->role != "admin")
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "forbidden";
            error->message = "Admin access required";
            return createDtoResponse(Status::CODE_403, error);
        }

        auto& authManager = auth::AuthManager::get();

        if (authManager.is_admin(username->c_str()))
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "forbidden";
            error->message = "Cannot delete admin user";
            return createDtoResponse(Status::CODE_400, error);
        }

        if (!authManager.remove_user(username->c_str()))
        {
            auto error = ErrorResponseDto::createShared();
            error->error = "not_found";
            error->message = "User not found";
            return createDtoResponse(Status::CODE_404, error);
        }

        auto response = LogoutResponseDto::createShared();
        response->message = "User deleted successfully";
        return createDtoResponse(Status::CODE_200, response);
    }

private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);
};

#include OATPP_CODEGEN_END(ApiController)
