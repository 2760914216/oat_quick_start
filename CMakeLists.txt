cmake_minimum_required(VERSION 3.10.0)
project(oat_quick_start VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL  REQUIRED)
find_package(oatpp          1.4.0 REQUIRED)
find_package(oatpp-openssl  1.4.0 REQUIRED)
# find_package(SQLite3              REQUIRED)
# find_package(oatpp-swagger  1.4.0 REQUIRED)
# find_package(oatpp-sqlite   1.4.0 REQUIRED)

include_directories(src)

add_executable(oat_quick_start src/App.cpp)

target_link_libraries(oat_quick_start 
        # Oat++
        PUBLIC oatpp::oatpp
        PUBLIC oatpp::oatpp-openssl
)