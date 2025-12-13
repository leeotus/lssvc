include_directories("${LSSVC_INCLUDE_PATH}/network/base")
include_directories("${LSSVC_INCLUDE_PATH}/network/net")

file(GLOB NETWORK_BASE_HEADERS "${LSSVC_INCLUDE_PATH}/network/base/**.h")
file(GLOB NETWORK_NET_HEADERS "${LSSVC_INCLUDE_PATH}/network/net/**.h")
file(GLOB NETWORK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/**.cpp")

add_library(
  ${PROJECT_NAME}-net
  ${NETWORK_BASE_HEADERS}
  ${NETWORK_NET_HEADERS}
  ${NETWORK_SOURCES}
)

add_library(
  lssvc::net ALIAS ${PROJECT_NAME}-net
)

set_target_properties(
  ${PROJECT_NAME}-net PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  EXPORT_NAME net
)
