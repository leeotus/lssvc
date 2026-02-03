# utils
include_directories("${LSSVC_INCLUDE_PATH}/utils")

# network
include_directories("${LSSVC_INCLUDE_PATH}/network")
include_directories("${LSSVC_INCLUDE_PATH}/network/base")
include_directories("${LSSVC_INCLUDE_PATH}/network/net")

# mmedia
include_directories("${LSSVC_INCLUDE_PATH}/mmedia")
include_directories("${LSSVC_INCLUDE_PATH}/mmedia/base")
include_directories("${LSSVC_INCLUDE_PATH}/mmedia/rtmp")    # rtmp

file(GLOB MMEDIA_CURRENT_HEADERS "${LSSVC_INCLUDE_PATH}/mmedia/**.h")
file(GLOB MMEDIA_CURRENT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/**.cpp")

# mmedia/base
file(GLOB MMEDIA_BASE_HEADERS "${LSSVC_INCLUDE_PATH}/mmedia/base/**.h")
file(GLOB MMEDIA_BASE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/base/**.cpp")

# mmedia/rtmp
file(GLOB MMEDIA_RTMP_HEADERS "${LSSVC_INCLUDE_PATH}/mmedia/rtmp/**.h")
file(GLOB MMEDIA_RTMP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/rtmp/**.cpp")

# mmedia/rtmp/amf
file(GLOB MMEDIA_RTMP_AMF_HEADERS "${LSSVC_INCLUDE_PATH}/mmedia/rtmp/amf/**.h")
file(GLOB MMEDIA_RTMP_AMF_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/rtmp/amf/**.cpp")

# mmedia/http
file(GLOB MMEDIA_HTTP_HEADERS "${LSSVC_INCLUDE_PATH}/mmedia/http/**.h")
file(GLOB MMEDIA_HTTP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/http/**.cpp")

file(GLOB MMEDIA_FLV_HEADERS "${LSSVC_INCLUDE_PATH}/mmedia/flv/**.h")
file(GLOB MMEDIA_FLV_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/flv/**.cpp")

# openssl
find_package(OpenSSL REQUIRED)

add_library(
  ${PROJECT_NAME}-mmedia

  ${MMEDIA_CURRENT_HEADERS}
  ${MMEDIA_BASE_HEADERS}
  ${MMEDIA_CURRENT_SOURCES}
  ${MMEDIA_BASE_SOURCES}

  # rtmp
  ${MMEDIA_RTMP_HEADERS}
  ${MMEDIA_RTMP_SOURCES}

  # rtmp/amf
  ${MMEDIA_RTMP_AMF_HEADERS}
  ${MMEDIA_RTMP_AMF_SOURCES}

  # http
  ${MMEDIA_HTTP_HEADERS}
  ${MMEDIA_HTTP_SOURCES}

  # flv
  ${MMEDIA_FLV_HEADERS}
  ${MMEDIA_FLV_SOURCES}
)

target_link_libraries(${PROJECT_NAME}-mmedia PRIVATE lssvc::utils lssvc::net OpenSSL::SSL OpenSSL::Crypto)

add_library(
  lssvc::mmedia ALIAS ${PROJECT_NAME}-mmedia
)

set_target_properties(
  ${PROJECT_NAME}-mmedia PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  EXPORT_NAME mmedia
)
