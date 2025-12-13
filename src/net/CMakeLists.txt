include_directories(${LSSVC_INCLUDE_PATH}/net)

file(GLOB NET_HEADERS "${LSSVC_INCLUDE_PATH}/net/**.h")
file(GLOB NET_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/**.cpp")

add_library(
  net
  ${NET_HEADERS}
  ${NET_SOURCES}
)

add_library(
  lssvc::net ALIAS net
)

set_target_properties(
  net PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  EXPORT_NAME net
)
