#include "AppComponent.hpp"

#include "controller/AuthController.hpp"
#include "controller/PhotoController.hpp"
#include "controller/UserController.hpp"
#include "oatpp/network/Server.hpp"

#include <oatpp/Environment.hpp>
#include <oatpp/base/Log.hpp>
#include <openssl/ssl.h>

constexpr char AppName[] = "Ciallo";

void run()
{
    AppComponent components;

    OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);

    auto authController = std::make_shared<AuthController>();
    router->addController(authController);

    auto userController = std::make_shared<UserController>();
    router->addController(userController);

    auto photoController = std::make_shared<PhotoController>();
    router->addController(photoController);

    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler);
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);

    oatpp::network::Server server(connectionProvider, connectionHandler);

    OATPP_LOGi(AppName, "Server running on port {}...", connectionProvider->getProperty("port").toString());

    server.run();
}

int main(int argc, char** argv)
{
    oatpp::Environment::init();
    run();
    oatpp::Environment::destroy();
    return 0;
}
