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
    DTO_INIT(PublicKeyDto, DTO)
    DTO_FIELD(String, owner); //
    DTO_FIELD(String, key);   //
    DTO_FIELD(String, algorithm);
};

class PublicKeyResponseDto : public oatpp::DTO
{
    DTO_INIT(PublicKeyResponseDto, DTO)
    DTO_FIELD(String, key);
};

class SymmetricKeyDto : public oatpp::DTO
{
    DTO_INIT(SymmetricKeyDto, DTO)
    DTO_FIELD(String, key);
};

class SymmetricKeyResponseDto : public oatpp::DTO
{
    DTO_INIT(SymmetricKeyResponseDto, DTO)
    DTO_FIELD(String, key);
};

class UploadInitRequestDto : public oatpp::DTO
{
    DTO_INIT(UploadInitRequestDto, DTO)
    DTO_FIELD(String, filename);        // 文件名
    DTO_FIELD(String, filetype);        // 文件类型
    DTO_FIELD(Int64, filesize);         // 文件大小
    DTO_FIELD(Int64, chunksize);        // 块大小
    DTO_FIELD(Int64, chunkcount);       // 块数量
    DTO_FIELD(String, encryptedSymKey); // 用服务器公钥加密的对称密钥 (Base64 编码)
};

class UploadInitResponseDto : public oatpp::DTO
{
    DTO_INIT(UploadInitResponseDto, DTO)
    DTO_FIELD(String, token);    // 上传令牌
    DTO_FIELD(Int64, chunkSize); // 建议的分块大小
    DTO_FIELD(String, message);  // 响应消息
};

class UploadChunkRequestDto : public oatpp::DTO
{
    DTO_INIT(UploadChunkRequestDto, DTO)
    DTO_FIELD(Int32, chunkIndex);      // 分块索引
    DTO_FIELD(String, encryptedChunk); // 用对称密钥加密的分块数据 (Base64 编码)
    DTO_FIELD(String, chunkHash);      // 分块的哈希值
};

class UploadChunkResponseDto : public oatpp::DTO
{
    DTO_INIT(UploadChunkRequestDto, DTO)
    DTO_FIELD(Int32, chunkIndex); // 分块索引
};

class UploadCompleteRequestDto : public oatpp::DTO
{
    DTO_INIT(UploadCompleteRequestDto, DTO)
    DTO_FIELD(String, token);
    DTO_FIELD(String, filename);
    DTO_FIELD(Int64, chunkCount);
};

class UploadCompleteResponseDto : public oatpp::DTO
{
    DTO_INIT(UploadCompleteResponseDto, DTO)
    DTO_FIELD(String, fileId);  // 文件 ID
    DTO_FIELD(String, message); // 完成消息
};

/* TODO - Add more DTOs here */

/* End DTO code-generation */
#include OATPP_CODEGEN_END(DTO)