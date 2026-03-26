#pragma once

#include "oatpp/macro/codegen.hpp"
#include "oatpp/network/Server.hpp"
#include <oatpp/Types.hpp>

/* Begin DTO code-generation */
#include OATPP_CODEGEN_BEGIN(DTO)

/**
 * Message Data-Transfer-Object
 */
class MessageDto : public oatpp::DTO
{

    DTO_INIT(MessageDto, DTO /* Extends */)

    DTO_FIELD(Int32, statusCode); // Status code field
    DTO_FIELD(String, message);   // Message field
    DTO_FIELD(String, CRC);       // CRC field
};

class PublicKeyDto : public oatpp::DTO
{
    DTO_INIT(PublicKeyDto,DTO)
    DTO_FIELD(String,owner);        //
    DTO_FIELD(String,key);          //
	DTO_FIELD(String,algorithm);
};

class PublicKeyResponseDto:public oatpp::DTO
{
    DTO_INIT(PublicKeyResponseDto, DTO)
    DTO_FIELD(String,key);
};

/* TODO - Add more DTOs here */

/* End DTO code-generation */
#include OATPP_CODEGEN_END(DTO)