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
    ENDPOINT("GET", "/hello", hello)
    {
        auto dto = MessageDto::createShared();
        dto->statusCode = 200;
        dto->message = "Ciallo World!";
        dto->CRC = "35c9bb";
        return createDtoResponse(Status::CODE_200, dto);
    }
    ENDPOINT("POST", "/api/submit-sensitive-info", publickey_transfer, BODY_DTO(Object<PublicKeyDto>, pubkey))
    {
        auto owner = pubkey->owner;
        auto key = pubkey->key;
        auto algorithm = pubkey->algorithm;
        auto dto = PublicKeyResponseDto::createShared();
        dto->key = "";
        if (algorithm == "RSA")
        {
            dto->key = key; // send it back, double check
            /*
             TODO store key in db
            */
            return createDtoResponse(Status::CODE_200, dto);
        }
        return createDtoResponse(Status::CODE_400, dto);
    }
    ENDPOINT("POST", "/api/submit-sensitive-info", symmetrickey_transfer, BODY_DTO(Object<SymmetricKeyDto>, symkey))
    {
        auto key = symkey->key;
        auto dto = SymmetricKeyResponseDto::createShared();
        dto->key = key;
        return createDtoResponse(Status::CODE_200, dto);
    }
    ENDPOINT("GET", "/api/get-public-key", server_publickey_transfer)
    {
        auto dto = PublicKeyDto::createShared();
        dto->owner = "server";
        // TODO get public key from db
        //  dto->algorithm=
        //  dto->key=
        return createDtoResponse(Status::CODE_200, dto);
    }
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