#pragma once

#include "auth/JwtHelper.hpp"
#include "db/PhotoMetadataDb.hpp"
#include "dto/AuthDTOs.hpp"
#include "dto/DTOs.hpp"
#include "dto/PhotoDTOs.hpp"
#include "resmanage/ResManager.hpp"

#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include <chrono>
#include <sstream>

#include OATPP_CODEGEN_BEGIN(ApiController)

class PhotoController : public oatpp::web::server::api::ApiController
{
public:
    PhotoController(OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper))
        : oatpp::web::server::api::ApiController(objectMapper), m_objectMapper(objectMapper)
    {
    }

    static oatpp::String get_current_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
        return ss.str().c_str();
    }

    std::optional<auth::TokenPayload> verify_auth(const std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>& request)
    {
        auto authHeader = request->getHeader("Authorization");
        if (!authHeader)
        {
            return std::nullopt;
        }
        auto headerStr = std::string(authHeader->c_str());
        if (headerStr.length() < 7 || headerStr.substr(0, 7) != "Bearer ")
        {
            return std::nullopt;
        }
        auto token = headerStr.substr(7);
        auto& jwt = auth::JwtHelper::get();
        return jwt.verify_token(token);
    }

public:
    ENDPOINT("GET", "/api/photos", listPhotos, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request))
    {
        auto payload = verify_auth(request);
        if (!payload)
        {
            return createResponse(Status::CODE_401);
        }

        auto photos = db::PhotoMetadataDb::get_instance().get_photos_by_owner(payload->username);
        auto response = PhotoListDto::createShared();
        response->photos = oatpp::Vector<Object<PhotoInfoDto>>::createShared();

        for (const auto& photo : photos)
        {
            auto photoDto = PhotoInfoDto::createShared();
            photoDto->id = photo.id.c_str();
            photoDto->filename = photo.filename.c_str();
            photoDto->size = photo.size;
            photoDto->created_at = photo.created_at.c_str();
            response->photos->push_back(photoDto);
        }

        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("POST", "/api/photos/upload", uploadPhoto, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request))
    {
        auto payload = verify_auth(request);
        if (!payload)
        {
            return createResponse(Status::CODE_401);
        }

        auto contentLength = request->getHeader("Content-Length");
        if (!contentLength)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "BAD_REQUEST";
            errorResponse->message = "Missing Content-Length header";
            return createDtoResponse(Status::CODE_400, errorResponse);
        }

        auto size = std::stoll(std::string(contentLength->c_str()));
        if (size <= 0 || size > 100 * 1024 * 1024)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "BAD_REQUEST";
            errorResponse->message = "Invalid file size";
            return createDtoResponse(Status::CODE_400, errorResponse);
        }

        auto contentType = request->getHeader("Content-Type");
        std::string extension = ".bin";

        if (contentType)
        {
            std::string ct(contentType->c_str());
            if (ct.find("image/jpeg") != std::string::npos || ct.find("image/jpg") != std::string::npos)
            {
                extension = ".jpg";
            }
            else if (ct.find("image/png") != std::string::npos)
            {
                extension = ".png";
            }
            else if (ct.find("image/gif") != std::string::npos)
            {
                extension = ".gif";
            }
            else if (ct.find("image/webp") != std::string::npos)
            {
                extension = ".webp";
            }
        }

        std::string filenameStr = "upload_" + res::ResManager::get_instance().generate_photo_id() + extension;
        oatpp::String filename = filenameStr.c_str();

        auto bodyStream = std::make_shared<oatpp::data::stream::BufferOutputStream>();
        request->transferBodyToStream(bodyStream);

        auto data = bodyStream->toString();
        std::vector<char> fileData(data->data(), data->data() + data->size());

        auto& resManager = res::ResManager::get_instance();
        auto writeResult = resManager.write_resource(res::ResManager::ResourceType::Photo, filenameStr, fileData);
        if (!writeResult || !*writeResult)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "INTERNAL_ERROR";
            errorResponse->message = "Failed to save file";
            return createDtoResponse(Status::CODE_500, errorResponse);
        }

        std::string photoId = resManager.generate_photo_id();
        std::string filepath = resManager.get_res_path(res::ResManager::ResourceType::Photo) + filenameStr;

        db::PhotoMetadata metadata;
        metadata.id = photoId;
        metadata.filename = filenameStr;
        metadata.filepath = filepath;
        metadata.size = static_cast<int64_t>(fileData.size());
        metadata.owner = payload->username;
        metadata.created_at = std::string(get_current_timestamp()->c_str());

        if (!db::PhotoMetadataDb::get_instance().insert_photo(metadata))
        {
            resManager.delete_resource(res::ResManager::ResourceType::Photo, filenameStr);
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "INTERNAL_ERROR";
            errorResponse->message = "Failed to save metadata";
            return createDtoResponse(Status::CODE_500, errorResponse);
        }

        auto response = UploadResponseDto::createShared();
        response->id = photoId.c_str();
        response->filename = filename;
        response->message = "Photo uploaded successfully";
        return createDtoResponse(Status::CODE_201, response);
    }

    ENDPOINT("POST", "/api/photos/init", initChunkUpload, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
             BODY_DTO(Object<UploadInitRequestDto>, req))
    {
        auto payload = verify_auth(request);
        if (!payload)
        {
            return createResponse(Status::CODE_401);
        }

        if (!req->filename || req->filename->empty())
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "BAD_REQUEST";
            errorResponse->message = "Filename is required";
            return createDtoResponse(Status::CODE_400, errorResponse);
        }
        if (!req->filesize || req->filesize <= 0)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "BAD_REQUEST";
            errorResponse->message = "Invalid filesize";
            return createDtoResponse(Status::CODE_400, errorResponse);
        }
        if (!req->chunkcount || req->chunkcount <= 0)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "BAD_REQUEST";
            errorResponse->message = "Invalid chunkcount";
            return createDtoResponse(Status::CODE_400, errorResponse);
        }

        auto& resManager = res::ResManager::get_instance();
        std::string sessionId = resManager.generate_photo_id();

        if (!resManager.create_temp_session(sessionId))
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "INTERNAL_ERROR";
            errorResponse->message = "Failed to create upload session";
            return createDtoResponse(Status::CODE_500, errorResponse);
        }

        auto response = UploadInitResponseDto::createShared();
        response->token = sessionId.c_str();
        response->chunkSize = 1024 * 1024;
        response->message = "Upload session created";
        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("POST", "/api/photos/chunk", uploadChunk, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
             BODY_DTO(Object<UploadChunkRequestDto>, req))
    {
        auto payload = verify_auth(request);
        if (!payload)
        {
            return createResponse(Status::CODE_401);
        }

        auto sessionHeader = request->getHeader("X-Upload-Token");
        if (!sessionHeader)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "BAD_REQUEST";
            errorResponse->message = "Missing X-Upload-Token header";
            return createDtoResponse(Status::CODE_400, errorResponse);
        }

        std::string sessionId(sessionHeader->c_str());
        auto& resManager = res::ResManager::get_instance();

        if (!req->encryptedChunk || req->encryptedChunk->empty())
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "BAD_REQUEST";
            errorResponse->message = "Empty chunk data";
            return createDtoResponse(Status::CODE_400, errorResponse);
        }

        std::vector<char> data(req->encryptedChunk->data(), req->encryptedChunk->data() + req->encryptedChunk->size());
        auto writeResult = resManager.write_chunk(sessionId, req->chunkIndex, data);
        if (!writeResult || !*writeResult)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "INTERNAL_ERROR";
            errorResponse->message = "Failed to save chunk";
            return createDtoResponse(Status::CODE_500, errorResponse);
        }

        auto response = UploadChunkResponseDto::createShared();
        response->chunkIndex = req->chunkIndex;
        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("POST", "/api/photos/complete", completeChunkUpload, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
             BODY_DTO(Object<UploadCompleteRequestDto>, req))
    {
        auto payload = verify_auth(request);
        if (!payload)
        {
            return createResponse(Status::CODE_401);
        }

        if (!req->token || req->token->empty())
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "BAD_REQUEST";
            errorResponse->message = "Token is required";
            return createDtoResponse(Status::CODE_400, errorResponse);
        }
        if (!req->filename || req->filename->empty())
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "BAD_REQUEST";
            errorResponse->message = "Filename is required";
            return createDtoResponse(Status::CODE_400, errorResponse);
        }

        auto& resManager = res::ResManager::get_instance();
        std::string photoId = resManager.generate_photo_id();
        std::string outputFilename = photoId + "_" + std::string(req->filename->c_str());
        std::string outputPath = resManager.get_res_path(res::ResManager::ResourceType::Photo) + outputFilename;
        int chunkCount = req->chunkCount ? static_cast<int>(req->chunkCount) : 0;

        auto mergeResult = resManager.merge_chunks(std::string(req->token->c_str()), outputPath, chunkCount);
        if (!mergeResult || !*mergeResult)
        {
            resManager.cleanup_session(std::string(req->token->c_str()));
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "INTERNAL_ERROR";
            errorResponse->message = "Failed to merge chunks";
            return createDtoResponse(Status::CODE_500, errorResponse);
        }

        auto fileSize = resManager.get_file_size(res::ResManager::ResourceType::Photo, outputFilename);
        resManager.cleanup_session(std::string(req->token->c_str()));

        db::PhotoMetadata metadata;
        metadata.id = photoId;
        metadata.filename = outputFilename;
        metadata.filepath = outputPath;
        metadata.size = fileSize;
        metadata.owner = payload->username;
        metadata.created_at = std::string(get_current_timestamp()->c_str());

        if (!db::PhotoMetadataDb::get_instance().insert_photo(metadata))
        {
            resManager.delete_resource(res::ResManager::ResourceType::Photo, outputFilename);
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "INTERNAL_ERROR";
            errorResponse->message = "Failed to save metadata";
            return createDtoResponse(Status::CODE_500, errorResponse);
        }

        auto response = UploadCompleteResponseDto::createShared();
        response->fileId = photoId.c_str();
        response->message = "Upload completed successfully";
        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("GET", "/api/photos/{id}", getPhoto, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request), PATH(String, id))
    {
        auto payload = verify_auth(request);
        if (!payload)
        {
            return createResponse(Status::CODE_401);
        }

        auto photoOpt = db::PhotoMetadataDb::get_instance().get_photo_by_id(std::string(id->c_str()));
        if (!photoOpt)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "NOT_FOUND";
            errorResponse->message = "Photo not found";
            return createDtoResponse(Status::CODE_404, errorResponse);
        }

        auto& photo = *photoOpt;
        if (photo.owner != payload->username && payload->role != "admin")
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "FORBIDDEN";
            errorResponse->message = "Access denied";
            return createDtoResponse(Status::CODE_403, errorResponse);
        }

        auto photoDto = PhotoInfoDto::createShared();
        photoDto->id = photo.id.c_str();
        photoDto->filename = photo.filename.c_str();
        photoDto->size = photo.size;
        photoDto->created_at = photo.created_at.c_str();

        return createDtoResponse(Status::CODE_200, photoDto);
    }

    ENDPOINT("GET", "/api/photos/{id}/file", downloadPhoto, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
             PATH(String, id))
    {
        auto payload = verify_auth(request);
        if (!payload)
        {
            return createResponse(Status::CODE_401);
        }

        auto photoOpt = db::PhotoMetadataDb::get_instance().get_photo_by_id(std::string(id->c_str()));
        if (!photoOpt)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "NOT_FOUND";
            errorResponse->message = "Photo not found";
            return createDtoResponse(Status::CODE_404, errorResponse);
        }

        auto& photo = *photoOpt;
        if (photo.owner != payload->username && payload->role != "admin")
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "FORBIDDEN";
            errorResponse->message = "Access denied";
            return createDtoResponse(Status::CODE_403, errorResponse);
        }

        auto& resManager = res::ResManager::get_instance();
        auto fileData = resManager.read_resource(res::ResManager::ResourceType::Photo, photo.filename);
        if (!fileData)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "NOT_FOUND";
            errorResponse->message = "File not found on disk";
            return createDtoResponse(Status::CODE_404, errorResponse);
        }

        auto response = createResponse(Status::CODE_200);
        response->putHeader("Content-Type", "application/octet-stream");
        response->putHeader("Content-Disposition", "attachment; filename=\"" + photo.filename + "\"");
        response->putHeader("Content-Length", std::to_string(fileData->size()));
        auto str = std::make_shared<std::string>(std::string(fileData->begin(), fileData->end()));
        return response;
    }

    ENDPOINT("DELETE", "/api/photos/{id}", deletePhoto, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
             PATH(String, id))
    {
        auto payload = verify_auth(request);
        if (!payload)
        {
            return createResponse(Status::CODE_401);
        }

        auto photoOpt = db::PhotoMetadataDb::get_instance().get_photo_by_id(std::string(id->c_str()));
        if (!photoOpt)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "NOT_FOUND";
            errorResponse->message = "Photo not found";
            return createDtoResponse(Status::CODE_404, errorResponse);
        }

        auto& photo = *photoOpt;
        if (photo.owner != payload->username && payload->role != "admin")
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "FORBIDDEN";
            errorResponse->message = "Access denied";
            return createDtoResponse(Status::CODE_403, errorResponse);
        }

        auto& resManager = res::ResManager::get_instance();
        resManager.delete_resource(res::ResManager::ResourceType::Photo, photo.filename);
        db::PhotoMetadataDb::get_instance().delete_photo(std::string(id->c_str()));

        auto response = DeleteResponseDto::createShared();
        response->message = "Photo deleted successfully";
        return createDtoResponse(Status::CODE_200, response);
    }

private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> m_objectMapper;
};

#include OATPP_CODEGEN_END(ApiController)
