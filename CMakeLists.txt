cmake_minimum_required(VERSION 3.0.0)
project(multi_thread_downloader VERSION 0.1.0 LANGUAGES C CXX)

#支持c++20标准
set(CMAKE_CXX_STANDARD 20)

include(CTest)
enable_testing()

file(GLOB SOURCES "*.cpp" "*.h")
add_executable(${PROJECT_NAME} ${SOURCES})

#input your vcpkg path
set(CMAKE_PREFIX_PATH "/home/cyq/vcpkg/packages")

find_package(cryptopp CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(fmt CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} pthread CURL::libcurl cryptopp::cryptopp fmt::fmt)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
