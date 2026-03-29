#pragma once

#include "oatpp/macro/codegen.hpp"
#include "oatpp/data/mapping/TypeResolver.hpp"
#include "oatpp/json/ObjectMapper.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class LoginRequestDto : public oatpp::DTO
{
    DTO_INIT(LoginRequestDto, DTO)
    DTO_FIELD(String, username);
    DTO_FIELD(String, password);
};

class LoginResponseDto : public oatpp::DTO
{
    DTO_INIT(LoginResponseDto, DTO)
    DTO_FIELD(String, access_token);
    DTO_FIELD(String, refresh_token);
    DTO_FIELD(Int32, expires_in);
    DTO_FIELD(String, username);
    DTO_FIELD(String, role);
};

class RefreshRequestDto : public oatpp::DTO
{
    DTO_INIT(RefreshRequestDto, DTO)
    DTO_FIELD(String, refresh_token);
};

class RefreshResponseDto : public oatpp::DTO
{
    DTO_INIT(RefreshResponseDto, DTO)
    DTO_FIELD(String, access_token);
    DTO_FIELD(Int32, expires_in);
};

class LogoutResponseDto : public oatpp::DTO
{
    DTO_INIT(LogoutResponseDto, DTO)
    DTO_FIELD(String, message);
};

class ErrorResponseDto : public oatpp::DTO
{
    DTO_INIT(ErrorResponseDto, DTO)
    DTO_FIELD(String, error);
    DTO_FIELD(String, message);
};

class UserDto : public oatpp::DTO
{
    DTO_INIT(UserDto, DTO)
    DTO_FIELD(String, username);
    DTO_FIELD(String, created_at);
};

class UserListDto : public oatpp::DTO
{
    DTO_INIT(UserListDto, DTO)
    DTO_FIELD(Vector<Object<UserDto>>, users);
};

class CreateUserRequestDto : public oatpp::DTO
{
    DTO_INIT(CreateUserRequestDto, DTO)
    DTO_FIELD(String, username);
    DTO_FIELD(String, password);
};

#include OATPP_CODEGEN_END(DTO)
