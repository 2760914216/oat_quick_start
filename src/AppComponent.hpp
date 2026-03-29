#pragma once

#include "oatpp/json/ObjectMapper.hpp"

#include "oatpp/web/server/HttpConnectionHandler.hpp"

#include "oatpp/macro/component.hpp"
#include <cstdlib>
#include <memory>
#include <oatpp-openssl/Config.hpp>
#include <oatpp-openssl/server/ConnectionProvider.hpp>
#include <fstream>

/**
 *  Class which creates and holds Application components and registers components in oatpp::base::Environment
 *  Order of components initialization is from top to bottom
 */

// const char* pemFile = "ca/private_key.pem";
// const char* crtFile = "ca/certificate.crt";
// auto config = oatpp::openssl::Config::createDefaultServerConfigShared(pemFile, crtFile);
// auto connectionProvider = oatpp::openssl::server::ConnectionProvider::createShared(config, {"localhost", 1145});
class AppComponent
{
private:
    static bool fileExists(const std::string& filepath)
    {
        std::ifstream f(filepath);
        return f.good();
    }

public:
    /**
     *  SSL Config component
     */
    // OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::openssl::Config>, opensslConfig)
    // (
    //     []
    //     {
    //         const char* pemFile = "/home/ruruka/Documents/srccode/oat_quick_start/ca/private_key.pem";
    //         const char* crtFile = "/home/ruruka/Documents/srccode/oat_quick_start/ca/certificate.crt";
    //         // system((std::string("cat ")+std::string(pemFile)).c_str());
    //         return oatpp::openssl::Config::createDefaultServerConfigShared(pemFile, crtFile);
    //     }());

    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::openssl::Config>, opensslConfig)
    (
        []
        {
            const char* pemFile = "/home/ruruka/Documents/srccode/oat_quick_start/ca/private_key.pem";
            const char* crtFile = "/home/ruruka/Documents/srccode/oat_quick_start/ca/certificate.crt";

            // std::cout << "[DEBUG] Checking files..." << std::endl;
            // std::cout << "[DEBUG] PEM exists: " << fileExists(pemFile) << std::endl;
            // std::cout << "[DEBUG] CRT exists: " << fileExists(crtFile) << std::endl;

            // if (!fileExists(pemFile) || !fileExists(crtFile))
            // {
            //     throw std::runtime_error("SSL certificate files not found!");
            // }

            // std::cout << "[DEBUG] Creating config with createShared()..." << std::endl;

            // use createShared rather then createDefaultServerConfigShared
            auto config = oatpp::openssl::Config::createDefaultServerConfigShared(crtFile,pemFile);
            // std::cout << "[DEBUG] Config created successfully!" << std::endl;
            return config;
        }());

    /**
     *  Create ConnectionProvider component which listens on the portreturn oatpp::network::tcp::server::ConnectionProvider::createShared({"localhost", 1145,
     * oatpp::network::Address::IP_4});
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)

    (
        []
        {
            OATPP_COMPONENT(std::shared_ptr<oatpp::openssl::Config>, config);
            // Create secure server connection provider
            return oatpp::openssl::server::ConnectionProvider::createShared(config, {"localhost", 1145, oatpp::network::Address::IP_4});
        }());
    /**
     *  Create Router component
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([] { return oatpp::web::server::HttpRouter::createShared(); }());

    /**
     *  Create ConnectionHandler component which uses Router component to route requests
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, serverConnectionHandler)
    (
        []
        {
            OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router); // get Router component
            return oatpp::web::server::HttpConnectionHandler::createShared(router);
        }());

    /**
     *  Create ObjectMapper component to serialize/deserialize DTOs in Contoller's API
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, apiObjectMapper)
    ([] { return std::shared_ptr<oatpp::json::ObjectMapper>(new oatpp::json::ObjectMapper()); }());
};
