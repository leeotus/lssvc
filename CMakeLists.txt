cmake_minimum_required(VERSION 3.0)
project(lssvc LANGUAGES C CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(LSSVC_INCLUDE_PATH "include")
set(LSSVC_SOURCE_PATH "src")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LSSVC_3RDPARTY_PATH "third_party")

if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.2.9)
    message(STATUS "GCC Version > 7.3.0, USING -Wno-error=unused-result -Werror=return-type")
    set(CMAKE_CXX_FLAGS "-Wno-error=unused-result -Werror=return-type ${CMAKE_CXX_FLAGS}")
endif()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -pipe -Wno-unused-variable -lrt -pthread")
elseif (NOT CMAKE_BUILD_TYPE STREQUAL Debug)
    message(STATUS "Release Version.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -pipe -Wno-unused-variable -lrt -pthread")
endif ()

include_directories("${LSSVC_INCLUDE_PATH}")
include_directories("${LSSVC_3RDPARTY_PATH}/jsoncpp/include")
link_directories(${CMAKE_BINARY_DIR}/third_party/jsoncpp/src/lib_json)
add_subdirectory("${LSSVC_3RDPARTY_PATH}/jsoncpp")  # compile jsoncpp first

add_subdirectory("${LSSVC_SOURCE_PATH}")

option(TEST_ENABLE "also build the testcases" ON)
if(${TEST_ENABLE})
  add_subdirectory(test)
endif()
