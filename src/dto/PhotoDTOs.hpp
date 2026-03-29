#pragma once

#include "oatpp/macro/codegen.hpp"
#include "oatpp/data/mapping/TypeResolver.hpp"
#include "oatpp/json/ObjectMapper.hpp"

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

#include OATPP_CODEGEN_END(DTO)
