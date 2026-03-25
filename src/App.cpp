#include "AppComponent.hpp"

#include "controller/MyController.hpp"
#include "oatpp/network/Server.hpp"

#include <oatpp/base/Log.hpp>

// #include "oatpp-openssl/Connection.hpp"

// /**
//  * Custom Request Handler
//  */
// class Handler : public oatpp::web::server::HttpRequestHandler
// {
// private:
//     /* Inject object mapper component */
//     OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, m_objectMapper);

// public:
//     /**
//      * Handle incoming request and return outgoing response.
//      */
//     std::shared_ptr<OutgoingResponse> handle(const std::shared_ptr<IncomingRequest>& request) override
//     {
//         auto message = MessageDto::createShared();
//         message->statusCode = 1024;
//         message->message = "Hello DTO!";
//         return ResponseFactory::createResponse(Status::CODE_200, message, m_objectMapper);
//     }
// };


const char* crtFile = "path/to/file.crt";
const char* pemFile = "path/to/file.pem";
// auto config = oatpp::libressl::Config::createDefaultServerConfigShared(crtFile, pemFile /* private key */);

void run()
{

    /* Register Components in scope of run() method */
    AppComponent components;

    /* Get router component */
    OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);

    // /* Route GET - "/hello" requests to Handler */
    // router->route("GET", "/hello", std::make_shared<Handler>());

    /* Create MyController and add all of its endpoints to router */
    auto myController = std::make_shared<MyController>();
    router->addController(myController);

    /* Get connection handler component */
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler);


    /* Get connection provider component */
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);

    /* Create server which takes provided TCP connections and passes them to HTTP connection handler */
    oatpp::network::Server server(connectionProvider, connectionHandler);

    /* Priny info about server port */
    OATPP_LOGi("MyApp", "Server running on port {}...", connectionProvider->getProperty("port").toString());

    /* Run server */
    server.run();

    // server.stop();
}

int main(int argc, char** argv)
{

    /* Init oatpp Environment */
    oatpp::Environment::init();

    /* Run App */
    run();

    /* Destroy oatpp Environment */
    oatpp::Environment::destroy();

    return 0;
}