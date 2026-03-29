cmake_minimum_required(VERSION 3.10.0)
project(oat_quick_start VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

option(BUILD_TESTS "Build unit tests" OFF)

find_package(OpenSSL REQUIRED)
find_package(oatpp          1.4.0 REQUIRED)
find_package(oatpp-openssl  1.4.0 REQUIRED)

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
endif()

include_directories(src)
include_directories(${OPENSSL_INCLUDE_DIR})

set(AUTH_SOURCES
    src/config/ConfigManager.cpp
    src/auth/AuthManager.cpp
    src/auth/JwtHelper.cpp
    src/auth/TokenStore.cpp
)

add_executable(oat_quick_start 
    src/App.cpp
    src/resmanage/ResManager.cpp
    ${AUTH_SOURCES}
)

target_link_libraries(oat_quick_start 
    oatpp::oatpp
    oatpp::oatpp-openssl
    ssl
    crypto
)

if(BUILD_TESTS)
    add_executable(res_manager_test 
        tests/ResManagerTest.cpp
        src/resmanage/ResManager.cpp
    )
    
    target_link_libraries(res_manager_test 
        GTest::GTest
        GTest::Main
        pthread
    )
    
    enable_testing()
    add_test(NAME ResManagerTest COMMAND res_manager_test)
endif()
