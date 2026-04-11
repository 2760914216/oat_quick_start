#pragma once

#include "auth/JwtHelper.hpp"
#include "classifier/ClassificationResult.hpp"
#include "classifier/ClassifierManager.hpp"
#include "db/PhotoMetadataDb.hpp"
#include "dto/AuthDTOs.hpp"
#include "dto/DTOs.hpp"
#include "dto/PhotoDTOs.hpp"
#include "image/ImageLoader.hpp"
#include "resmanage/ResManager.hpp"
#include "service/ImageEditor.hpp"
#include "service/ThumbnailGenerator.hpp"

#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include <chrono>
#include <fstream>
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

        // Auto-classification after upload
        try
        {
            auto imageData = ImageLoader::load(metadata.filepath);
            if (imageData)
            {
                auto& classifierMgr = ClassifierManager::getInstance();
                auto sceneClassifier = classifierMgr.getClassifier("scene");
                auto contentClassifier = classifierMgr.getClassifier("content");

                if (sceneClassifier && contentClassifier)
                {
                    // Update status to processing
                    metadata.classification_status = "processing";
                    db::PhotoMetadataDb::get_instance().update_photo(metadata);

                    // Run classification
                    std::string sceneCategory = "unknown";
                    std::string contentCategory = "unknown";

                    if (sceneClassifier->isModelLoaded())
                    {
                        auto sceneResult = sceneClassifier->classify(*imageData);
                        sceneCategory = sceneResult.category;
                    }

                    if (contentClassifier->isModelLoaded())
                    {
                        auto contentResult = contentClassifier->classify(*imageData);
                        contentCategory = contentResult.category;
                    }

                    // Update with results
                    metadata.scene_category = sceneCategory;
                    metadata.content_category = contentCategory;
                    metadata.classification_status = "completed";
                    metadata.classified_at = std::string(get_current_timestamp()->c_str());
                    db::PhotoMetadataDb::get_instance().update_photo(metadata);
                }
            }
        }
        catch (...)
        {
            // Classification failed, status remains pending
        }

        // Auto-generate thumbnails after upload
        try
        {
            auto imageData = ImageLoader::load(metadata.filepath);
            if (imageData)
            {
                // Generate list thumbnail (150x150)
                auto listThumbId = ThumbnailGenerator::generateThumbnail(photoId, *imageData, "list");
                // Generate detail thumbnail (400x400)
                auto detailThumbId = ThumbnailGenerator::generateThumbnail(photoId, *imageData, "detail");
            }
        }
        catch (...)
        {
            // Thumbnail generation failed, don't fail the upload
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

    ENDPOINT("GET", "/api/photos/{id}/thumbnail", getPhotoThumbnail, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
             PATH(String, id), QUERY(String, thumbSize))
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

        std::string sizeType = "list";
        if (thumbSize && !thumbSize->empty())
        {
            std::string s(thumbSize->c_str());
            if (s == "list" || s == "detail")
            {
                sizeType = s;
            }
        }

        auto existingThumbs = db::PhotoMetadataDb::get_instance().get_thumbnails_by_photo_id(photo.id);
        std::optional<std::string> thumbPath;

        for (const auto& thumb : existingThumbs)
        {
            if (thumb.size_type == sizeType)
            {
                thumbPath = thumb.thumbnail_path;
                break;
            }
        }

        if (!thumbPath)
        {
            auto imageData = ImageLoader::load(photo.filepath);
            if (!imageData)
            {
                auto errorResponse = ErrorResponseDto::createShared();
                errorResponse->error = "INTERNAL_ERROR";
                errorResponse->message = "Failed to load image";
                return createDtoResponse(Status::CODE_500, errorResponse);
            }

            auto thumbId = ThumbnailGenerator::generateThumbnail(photo.id, *imageData, sizeType);
            if (!thumbId)
            {
                auto errorResponse = ErrorResponseDto::createShared();
                errorResponse->error = "INTERNAL_ERROR";
                errorResponse->message = "Failed to generate thumbnail";
                return createDtoResponse(Status::CODE_500, errorResponse);
            }

            auto newThumbs = db::PhotoMetadataDb::get_instance().get_thumbnails_by_photo_id(photo.id);
            for (const auto& thumb : newThumbs)
            {
                if (thumb.size_type == sizeType)
                {
                    thumbPath = thumb.thumbnail_path;
                    break;
                }
            }
        }

        if (!thumbPath)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "INTERNAL_ERROR";
            errorResponse->message = "Thumbnail not found";
            return createDtoResponse(Status::CODE_500, errorResponse);
        }

        std::ifstream file(*thumbPath, std::ios::binary | std::ios::ate);
        if (!file)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "NOT_FOUND";
            errorResponse->message = "Thumbnail file not found";
            return createDtoResponse(Status::CODE_404, errorResponse);
        }

        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(fileSize);
        if (!file.read(buffer.data(), fileSize))
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "INTERNAL_ERROR";
            errorResponse->message = "Failed to read thumbnail";
            return createDtoResponse(Status::CODE_500, errorResponse);
        }

        std::string bodyData(buffer.begin(), buffer.end());
        auto response = createResponse(Status::CODE_200, bodyData.c_str());
        response->putHeader("Content-Type", "image/jpeg");
        response->putHeader("Content-Length", std::to_string(fileSize));
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

    ENDPOINT("GET", "/api/photos/{id}/classification", getPhotoClassification,
             REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request), PATH(String, id))
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

        if (photo.classification_status == "pending" || photo.classification_status == "processing")
        {
            auto response = ClassificationStatusDto::createShared();
            response->photo_id = photo.id.c_str();
            response->status = photo.classification_status.c_str();
            return createDtoResponse(Status::CODE_200, response);
        }
        else if (photo.classification_status == "completed")
        {
            auto response = ClassificationResultDto::createShared();
            response->photo_id = photo.id.c_str();
            response->scene_category = photo.scene_category.c_str();
            response->content_category = photo.content_category.c_str();
            response->confidence = 0.95f;
            response->classified_at = photo.classified_at.c_str();
            return createDtoResponse(Status::CODE_200, response);
        }
        else if (photo.classification_status == "failed")
        {
            auto response = ClassificationStatusDto::createShared();
            response->photo_id = photo.id.c_str();
            response->status = "failed";
            return createDtoResponse(Status::CODE_200, response);
        }

        auto response = ClassificationStatusDto::createShared();
        response->photo_id = photo.id.c_str();
        response->status = "pending";
        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("POST", "/api/photos/{id}/classify", classifyPhoto, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
             PATH(String, id), BODY_DTO(Object<ClassifyRequestDto>, req))
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

        bool forceClassify = req->force && req->force;

        if (!forceClassify && photo.classification_status == "completed")
        {
            auto response = ClassificationResultDto::createShared();
            response->photo_id = photo.id.c_str();
            response->scene_category = photo.scene_category.c_str();
            response->content_category = photo.content_category.c_str();
            response->confidence = 0.95f;
            response->classified_at = photo.classified_at.c_str();
            return createDtoResponse(Status::CODE_200, response);
        }

        db::PhotoMetadata updateMeta;
        updateMeta.id = photo.id;
        updateMeta.classification_status = "processing";
        db::PhotoMetadataDb::get_instance().update_photo(updateMeta);

        auto imageData = ImageLoader::load(photo.filepath);
        if (!imageData)
        {
            db::PhotoMetadata failedMeta;
            failedMeta.id = photo.id;
            failedMeta.classification_status = "failed";
            db::PhotoMetadataDb::get_instance().update_photo(failedMeta);

            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "INTERNAL_ERROR";
            errorResponse->message = "Failed to load image";
            return createDtoResponse(Status::CODE_500, errorResponse);
        }

        auto& classifierMgr = ClassifierManager::getInstance();

        std::string sceneCategory;
        std::string contentCategory;
        float confidence = 0.0f;

        auto sceneClassifier = classifierMgr.getClassifier("scene");
        if (sceneClassifier && sceneClassifier->isModelLoaded())
        {
            auto sceneResult = sceneClassifier->classify(*imageData);
            sceneCategory = sceneResult.category;
            confidence = sceneResult.confidence;
        }

        auto contentClassifier = classifierMgr.getClassifier("content");
        if (contentClassifier && contentClassifier->isModelLoaded())
        {
            auto contentResult = contentClassifier->classify(*imageData);
            contentCategory = contentResult.category;
            if (contentResult.confidence > confidence)
            {
                confidence = contentResult.confidence;
            }
        }

        if (sceneCategory.empty()) sceneCategory = "unknown";
        if (contentCategory.empty()) contentCategory = "unknown";

        std::string timestamp = std::string(get_current_timestamp()->c_str());

        db::PhotoMetadata completedMeta;
        completedMeta.id = photo.id;
        completedMeta.scene_category = sceneCategory;
        completedMeta.content_category = contentCategory;
        completedMeta.classification_status = "completed";
        completedMeta.classified_at = timestamp;
        db::PhotoMetadataDb::get_instance().update_photo(completedMeta);

        auto response = ClassificationResultDto::createShared();
        response->photo_id = photo.id.c_str();
        response->scene_category = sceneCategory.c_str();
        response->content_category = contentCategory.c_str();
        response->confidence = confidence;
        response->classified_at = timestamp.c_str();
        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("POST", "/api/photos/{id}/edit", editPhoto, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
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

        auto bodyStream = std::make_shared<oatpp::data::stream::BufferOutputStream>();
        request->transferBodyToStream(bodyStream);
        auto bodyData = bodyStream->toString();

        std::string jsonStr(bodyData->c_str(), bodyData->size());
        auto editParamsOpt = ImageEditor::fromJson(jsonStr);
        if (!editParamsOpt)
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "BAD_REQUEST";
            errorResponse->message = "Invalid edit parameters JSON";
            return createDtoResponse(Status::CODE_400, errorResponse);
        }

        auto normalizedParams = *editParamsOpt;
        if (normalizedParams.brightness < -20) normalizedParams.brightness = -20;
        if (normalizedParams.brightness > 20) normalizedParams.brightness = 20;
        if (normalizedParams.contrast < 0.5f) normalizedParams.contrast = 0.5f;
        if (normalizedParams.contrast > 2.0f) normalizedParams.contrast = 2.0f;
        if (normalizedParams.rotation != 0 && normalizedParams.rotation != 90 && normalizedParams.rotation != 180 && normalizedParams.rotation != 270)
        {
            normalizedParams.rotation = 0;
        }

        std::string editJson = ImageEditor::toJson(normalizedParams);
        std::string timestamp = std::string(get_current_timestamp()->c_str());

        db::PhotoEdit edit;
        edit.id = photo.id + "_edit";
        edit.photo_id = photo.id;
        edit.edit_params = editJson;
        edit.created_at = timestamp;
        edit.updated_at = timestamp;

        if (!db::PhotoMetadataDb::get_instance().insert_edit(edit))
        {
            auto errorResponse = ErrorResponseDto::createShared();
            errorResponse->error = "INTERNAL_ERROR";
            errorResponse->message = "Failed to save edit parameters";
            return createDtoResponse(Status::CODE_500, errorResponse);
        }

        auto response = EditResponseDto::createShared();
        response->photo_id = photo.id.c_str();
        response->edit_params = editJson.c_str();
        response->created_at = timestamp.c_str();
        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("GET", "/api/photos/{id}/edit", getPhotoEdit, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
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

        auto editOpt = db::PhotoMetadataDb::get_instance().get_edit_by_photo_id(photo.id);
        auto response = EditParamsResponseDto::createShared();

        if (editOpt)
        {
            auto editParams = *editOpt;
            auto editParamsObj = ImageEditor::fromJson(editParams.edit_params);
            if (editParamsObj)
            {
                auto dtoParams = EditRequestDto::createShared();
                dtoParams->cropX = editParamsObj->cropX;
                dtoParams->cropY = editParamsObj->cropY;
                dtoParams->cropWidth = editParamsObj->cropWidth;
                dtoParams->cropHeight = editParamsObj->cropHeight;
                dtoParams->rotation = editParamsObj->rotation;
                dtoParams->targetWidth = editParamsObj->targetWidth;
                dtoParams->targetHeight = editParamsObj->targetHeight;
                dtoParams->brightness = editParamsObj->brightness;
                dtoParams->contrast = editParamsObj->contrast;

                auto filterStr = std::string();
                switch (editParamsObj->filter)
                {
                case EditParams::Filter::NONE:
                    filterStr = "NONE";
                    break;
                case EditParams::Filter::GRAYSCALE:
                    filterStr = "GRAYSCALE";
                    break;
                case EditParams::Filter::SEPIA:
                    filterStr = "SEPIA";
                    break;
                case EditParams::Filter::VINTAGE:
                    filterStr = "VINTAGE";
                    break;
                default:
                    filterStr = "NONE";
                    break;
                }
                dtoParams->filter = filterStr.c_str();
                response->edit_params = dtoParams;
            }
            else
            {
                response->edit_params = nullptr;
            }
        }
        else
        {
            response->edit_params = nullptr;
        }

        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("POST", "/api/photos/{id}/revert", revertPhotoEdit, REQUEST(std::shared_ptr<oatpp::web::server::api::ApiController::IncomingRequest>, request),
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

        db::PhotoMetadataDb::get_instance().delete_edit(photo.id);

        auto response = RevertResponseDto::createShared();
        response->success = true;
        return createDtoResponse(Status::CODE_200, response);
    }

private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> m_objectMapper;
};

#include OATPP_CODEGEN_END(ApiController)
