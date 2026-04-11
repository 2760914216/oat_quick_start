cmake_minimum_required(VERSION 3.10.0)
project(oat_quick_start VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

option(BUILD_TESTS "Build unit tests" NO)
option(BUILD_API_TESTS "Build HTTP API tests" NO)
option(ENABLE_ONNX "Enable ONNX Runtime for image classification" ON)

find_package(OpenSSL REQUIRED)
find_package(oatpp          1.4.0 REQUIRED)
find_package(oatpp-openssl  1.4.0 REQUIRED)
find_package(oatpp-sqlite   1.4.0 REQUIRED)
find_package(SQLite3 REQUIRED)

# ONNX Runtime - optional
if(ENABLE_ONNX)
    find_package(ONNXRuntime)
    if(ONNXRuntime_FOUND)
        message(STATUS "ONNX Runtime found: ${ONNXRuntime_INCLUDE_DIRS}")
        add_definitions(-DENABLE_ONNX=1)
        include_directories(${ONNXRuntime_INCLUDE_DIRS})
    else()
        message(STATUS "ONNX Runtime not found, classification features disabled")
        add_definitions(-DENABLE_ONNX=0)
    endif()
endif()

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
    include_directories(${OATPP_INCLUDE_DIRS})
endif()

include_directories(src)
include_directories(src/image)
include_directories(src/service)
include_directories(src/db)
include_directories(src/classifier)
include_directories(${OPENSSL_INCLUDE_DIR})

set(AUTH_SOURCES
    src/config/ConfigManager.cpp
    src/auth/AuthManager.cpp
    src/auth/JwtHelper.cpp
    src/auth/TokenStore.cpp
)

set(STORAGE_SOURCES
    src/db/PhotoMetadataDb.cpp
)

set(IMAGE_SOURCES
    src/image/ImageLoader.cpp
)

set(SERVICE_SOURCES
    src/service/ThumbnailGenerator.cpp
    src/service/ImageEditor.cpp
)

set(CLASSIFIER_SOURCES
    src/classifier/ClassifierManager.cpp
    src/classifier/SceneClassifier.cpp
)

add_executable(oat_quick_start 
    src/App.cpp
    src/resmanage/ResManager.cpp
    ${AUTH_SOURCES}
    ${STORAGE_SOURCES}
    ${IMAGE_SOURCES}
    ${SERVICE_SOURCES}
    ${CLASSIFIER_SOURCES}
)

target_link_libraries(oat_quick_start 
    oatpp::oatpp
    oatpp::oatpp-openssl
    oatpp::oatpp-sqlite
    ssl
    crypto
    sqlite3
    ${ONNXRuntime_LIBRARIES}
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
    
    add_executable(image_classifier_test 
        tests/classifier/ImageClassifierTest.cpp
        src/classifier/ClassifierManager.cpp
        src/classifier/SceneClassifier.cpp
        src/classifier/ContentClassifier.cpp
    )
    
    target_link_libraries(image_classifier_test 
        GTest::GTest
        GTest::Main
        pthread
    )
    
    add_executable(image_service_test 
        tests/service/ImageServiceTest.cpp
        src/service/ImageEditor.cpp
    )
    
    target_link_libraries(image_service_test 
        GTest::GTest
        GTest::Main
        pthread
    )
    
    add_executable(photo_api_test 
        tests/api/PhotoApiTest.cpp
        src/service/ImageEditor.cpp
    )
    
    target_link_libraries(photo_api_test 
        GTest::GTest
        GTest::Main
        pthread
    )
    
    add_executable(workflow_test 
        tests/e2e/WorkflowTest.cpp
        src/db/PhotoMetadataDb.cpp
        src/service/ImageEditor.cpp
    )
    
    target_link_libraries(workflow_test 
        GTest::GTest
        GTest::Main
        pthread
        sqlite3
    )
    
    enable_testing()
    add_test(NAME ResManagerTest COMMAND res_manager_test)
    add_test(NAME ImageClassifierTest COMMAND image_classifier_test)
    add_test(NAME ImageServiceTest COMMAND image_service_test)
    add_test(NAME PhotoApiTest COMMAND photo_api_test)
    add_test(NAME WorkflowTest COMMAND workflow_test)
endif()

if(BUILD_API_TESTS)
    add_executable(api_test 
        tests/ApiTest.cpp
    )
    
    target_link_libraries(api_test 
        pthread
    )
    
    message(STATUS "API tests enabled - run './tests/test_api.sh' to test HTTP endpoints")
    message(STATUS "Note: Start server first with './build/oat_quick_start'")
endif()
