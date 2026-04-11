#pragma once

#include "oatpp/data/mapping/TypeResolver.hpp"
#include "oatpp/json/ObjectMapper.hpp"
#include "oatpp/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class PhotoInfoDto : public oatpp::DTO
{
    DTO_INIT(PhotoInfoDto, DTO)
    DTO_FIELD(String, id);
    DTO_FIELD(String, filename);
    DTO_FIELD(Int64, size);
    DTO_FIELD(String, created_at);
};

class PhotoListDto : public oatpp::DTO
{
    DTO_INIT(PhotoListDto, DTO)
    DTO_FIELD(Vector<Object<PhotoInfoDto>>, photos);
};

class UploadResponseDto : public oatpp::DTO
{
    DTO_INIT(UploadResponseDto, DTO)
    DTO_FIELD(String, id);
    DTO_FIELD(String, filename);
    DTO_FIELD(String, message);
};

class DeleteResponseDto : public oatpp::DTO
{
    DTO_INIT(DeleteResponseDto, DTO)
    DTO_FIELD(String, message);
};

class ClassificationStatusDto : public oatpp::DTO
{
    DTO_INIT(ClassificationStatusDto, DTO)
    DTO_FIELD(String, photo_id);
    DTO_FIELD(String, status);
};

class ClassificationResultDto : public oatpp::DTO
{
    DTO_INIT(ClassificationResultDto, DTO)
    DTO_FIELD(String, photo_id);
    DTO_FIELD(String, scene_category);
    DTO_FIELD(String, content_category);
    DTO_FIELD(Float32, confidence);
    DTO_FIELD(String, classified_at);
};

class ClassifyRequestDto : public oatpp::DTO
{
    DTO_INIT(ClassifyRequestDto, DTO)
    DTO_FIELD(Boolean, force);
};

class EditRequestDto : public oatpp::DTO
{
    DTO_INIT(EditRequestDto, DTO)
    // Crop parameters
    DTO_FIELD(Int32, cropX);
    DTO_FIELD(Int32, cropY);
    DTO_FIELD(Int32, cropWidth);
    DTO_FIELD(Int32, cropHeight);
    // Rotation: 0, 90, 180, 270
    DTO_FIELD(Int32, rotation);
    // Resize dimensions
    DTO_FIELD(Int32, targetWidth);
    DTO_FIELD(Int32, targetHeight);
    // Brightness adjustment: -20 to +20
    DTO_FIELD(Int32, brightness);
    // Contrast factor: 0.5 to 2.0
    DTO_FIELD(Float32, contrast);
    // Filter: NONE, GRAYSCALE, SEPIA, VINTAGE
    DTO_FIELD(String, filter);
};

class EditResponseDto : public oatpp::DTO
{
    DTO_INIT(EditResponseDto, DTO)
    DTO_FIELD(String, photo_id);
    DTO_FIELD(String, edit_params);
    DTO_FIELD(String, created_at);
};

class EditParamsResponseDto : public oatpp::DTO
{
    DTO_INIT(EditParamsResponseDto, DTO)
    DTO_FIELD(Object<EditRequestDto>, edit_params);
};

class RevertResponseDto : public oatpp::DTO
{
    DTO_INIT(RevertResponseDto, DTO)
    DTO_FIELD(Boolean, success);
};

#include OATPP_CODEGEN_END(DTO)
