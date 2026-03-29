#pragma once

#include "auth/JwtHelper.hpp"
#include "dto/AuthDTOs.hpp"
#include "dto/PhotoDTOs.hpp"
#include "resmanage/ResManager.hpp"

#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController)

class PhotoController : public oatpp::web::server::api::ApiController
{
public:
    PhotoController(OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper)) : oatpp::web::server::api::ApiController(objectMapper)
    {
    }

public:
    ENDPOINT("GET", "/api/photos", listPhotos, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request))
    {
        auto authHeader = request->getHeader("Authorization");
        auto headerStr = std::string(authHeader->c_str());
        auto token = headerStr.substr(7);

        auto& jwt = auth::JwtHelper::get();
        auto payload = jwt.verify_token(token);

        if (!payload)
        {
            return createResponse(Status::CODE_401);
        }

        auto response = PhotoListDto::createShared();
        response->photos = oatpp::Vector<Object<PhotoInfoDto>>::createShared();
        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("POST", "/api/photos/upload", uploadPhoto, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request))
    {
        auto authHeader = request->getHeader("Authorization");
        auto headerStr = std::string(authHeader->c_str());
        auto token = headerStr.substr(7);

        auto& jwt = auth::JwtHelper::get();
        auto payload = jwt.verify_token(token);

        if (!payload)
        {
            return createResponse(Status::CODE_401);
        }

        auto response = UploadResponseDto::createShared();
        response->message = "Photo upload endpoint (stub)";
        return createDtoResponse(Status::CODE_201, response);
    }

    ENDPOINT("GET", "/api/photos/{id}", getPhoto, PATH(String, id)) { return createResponse(Status::CODE_501); }

    ENDPOINT("DELETE", "/api/photos/{id}", deletePhoto, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
             PATH(String, id))
    {
        auto authHeader = request->getHeader("Authorization");
        auto headerStr = std::string(authHeader->c_str());
        auto token = headerStr.substr(7);

        auto& jwt = auth::JwtHelper::get();
        auto payload = jwt.verify_token(token);

        if (!payload)
        {
            return createResponse(Status::CODE_401);
        }

        auto response = DeleteResponseDto::createShared();
        response->message = "Photo delete endpoint (stub)";
        return createDtoResponse(Status::CODE_200, response);
    }

private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);
};

#include OATPP_CODEGEN_END(ApiController)
