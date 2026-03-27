cmake_minimum_required(VERSION 3.10.0)
project(oat_quick_start VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL REQUIRED)
find_package(oatpp          1.4.0 REQUIRED)
find_package(oatpp-openssl  1.4.0 REQUIRED)

include_directories(src)
include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(oat_quick_start src/App.cpp)

# 使用更明确的链接方式
target_link_libraries(oat_quick_start 
        oatpp::oatpp
        oatpp::oatpp-openssl
        ssl
        crypto
)

# 设置链接器标志，确保符号不被丢弃
# set_target_properties(oat_quick_start PROPERTIES
#     LINK_FLAGS "-Wl,--no-as-needed"
# )