#pragma once

#include "oatpp/json/ObjectMapper.hpp"

#include "oatpp/web/server/HttpConnectionHandler.hpp"

#include "oatpp/macro/component.hpp"
#include <cstdlib>
#include <fstream>
#include <memory>
#include <oatpp-openssl/Config.hpp>
#include <oatpp-openssl/server/ConnectionProvider.hpp>

#include "auth/JwtHelper.hpp"
#include "config/ConfigManager.hpp"

class AppComponent
{
private:
    static bool fileExists(const std::string& filepath)
    {
        std::ifstream f(filepath);
        return f.good();
    }

public:
    AppComponent()
    {
        auto& config = config::ConfigManager::get();
        if (!config.load("conf.json"))
        {
            OATPP_LOGw("Ciallo", "Failed to load conf.json, using defaults");
        }
        else
        {
            OATPP_LOGi("Ciallo", "Configuration loaded from conf.json");
            auto& jwt = auth::JwtHelper::get();
            jwt.set_secret(config.get_jwt_secret());
            OATPP_LOGi("Ciallo", "JWT secret configured");
        }
    }

    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::openssl::Config>, opensslConfig)
    (
        []
        {
            const char* pemFile = "/home/ruruka/Documents/srccode/oat_quick_start/ca/private_key.pem";
            const char* crtFile = "/home/ruruka/Documents/srccode/oat_quick_start/ca/certificate.crt";

            auto config = oatpp::openssl::Config::createDefaultServerConfigShared(crtFile, pemFile);
            return config;
        }());

    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)
    (
        []
        {
            OATPP_COMPONENT(std::shared_ptr<oatpp::openssl::Config>, config);
            auto& appConfig = config::ConfigManager::get();
            std::string host = appConfig.get_server_host();
            auto port = static_cast<unsigned short>(appConfig.get_server_port());
            return oatpp::openssl::server::ConnectionProvider::createShared(config, {host, port, oatpp::network::Address::IP_4});
        }());

    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([] { return oatpp::web::server::HttpRouter::createShared(); }());

    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, serverConnectionHandler)
    (
        []
        {
            OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);
            return oatpp::web::server::HttpConnectionHandler::createShared(router);
        }());

    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, apiObjectMapper)
    ([] { return std::shared_ptr<oatpp::json::ObjectMapper>(new oatpp::json::ObjectMapper()); }());
};
