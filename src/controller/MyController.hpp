#pragma once

#include "dto/DTOs.hpp"

#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController) ///< Begin Codegen

/**
 * Sample Api Controller.
 */
class MyController : public oatpp::web::server::api::ApiController
{
public:
    /**
     * Constructor with object mapper.
     * @param objectMapper - default object mapper used to serialize/deserialize DTOs.
     */
    MyController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper)) : oatpp::web::server::api::ApiController(objectMapper) {}

public:
    ENDPOINT("GET", "/hello", root)
    {
        auto dto = MessageDto::createShared();
        dto->statusCode = 200;
        dto->message = "Hello World!";
        dto->CRC = "35c9bb";
        return createDtoResponse(Status::CODE_200, dto);
    }
    ENDPOINT("PULL", "/tempfiles", publickey_transfer, BODY_DTO(Object<PublicKeyDto>, pubkey))
    {
        auto owner = pubkey->owner;
        auto key = pubkey->key;
        auto algorithm = pubkey->algorithm;
        auto plaintxt = pubkey->plaintext;
        auto dto = PublicKeyResponseDto::createShared();
        if (algorithm != "AES-256-CBC") // unsupported algorithm
        {
            dto->encryptedtext = plaintxt; // return plaintext
                                           // retuen
        }
        // TODO
        return createDtoResponse(Status::CODE_200, dto);
    }
    // TODO Insert Your endpoints here !!!
};

#include OATPP_CODEGEN_END(ApiController) ///< End Codegen

/*
Multipart File Upload

ENDPOINT("POST", "test/multipart-all", multipartFileTest,
REQUEST(std::shared_ptr<IncomingRequest>, request))
{
  auto multipart = std::make_shared<multipart::PartList>(request->getHeaders());
  multipart::Reader multipartReader(multipart.get());

  // Configure part readers
  multipartReader.setPartReader("part1", multipart::createInMemoryPartReader(256));
  multipartReader.setPartReader("part2", multipart::createFilePartReader("/path/to/save/file"));
  multipartReader.setDefaultPartReader(multipart::createInMemoryPartReader(16 * 1024));

  // Read multipart body
  request->transferBody(&multipartReader);

  // Process uploaded parts
  auto filePart = multipart->getNamedPart("part2");
  auto inputStream = filePart->getPayload()->openInputStream();

  return createResponse(Status::CODE_200, "OK");
}

Async File Upload

ENDPOINT_ASYNC("POST", "test/multipart-all", MultipartUpload)
{
  std::shared_ptr<multipart::PartList> m_multipart;

  Action act() override {
    m_multipart = std::make_shared<multipart::PartList>(request->getHeaders());
    auto multipartReader = std::make_shared<multipart::AsyncReader>(m_multipart);

    multipartReader->setPartReader("part1", multipart::createAsyncInMemoryPartReader(256));
    multipartReader->setPartReader("part2", multipart::createAsyncFilePartReader("/path/to/save/file"));

    return request->transferBodyAsync(multipartReader).next(yieldTo(&MultipartUpload::onUploaded));
  }

  Action onUploaded() {
    // Process uploaded files
    return _return(controller->createResponse(Status::CODE_200, "OK"));
  }
}

File Streaming

ENDPOINT("POST", "upload", uploadFile,
REQUEST(std::shared_ptr<IncomingRequest>, request))
{
  auto fileStream = std::make_shared<FileOutputStream>("/path/to/uploaded_file.bin");
  request->transferBodyToStream(fileStream);
  return createResponse(Status::CODE_200, "File uploaded");
}

*/