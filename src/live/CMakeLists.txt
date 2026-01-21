# utils
include_directories("${LSSVC_INCLUDE_PATH}/utils")

# network
include_directories("${LSSVC_INCLUDE_PATH}/network")
include_directories("${LSSVC_INCLUDE_PATH}/network/base")
include_directories("${LSSVC_INCLUDE_PATH}/network/net")

# mmedia
include_directories("${LSSVC_INCLUDE_PATH}/mmedia")
include_directories("${LSSVC_INCLUDE_PATH}/mmedia/base")
include_directories("${LSSVC_INCLUDE_PATH}/mmedia/rtmp")

# live
include_directories("${LSSVC_INCLUDE_PATH}/live")
include_directories("${LSSVC_INCLUDE_PATH}/live/base")

# live/base
file(GLOB LIVE_BASE_HEADERS "${LSSVC_INCLUDE_PATH}/live/**.h")
file(GLOB LIVE_BASE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/base/**.cpp")
file(GLOB LIVE_CURRENT_HEADERS "${LSSVC_INCLUDE_PATH}/live/**.h")
file(GLOB LIVE_CURRENT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/**.cpp")

add_library(
  ${PROJECT_NAME}-live

  ${LIVE_BASE_HEADERS}
  ${LIVE_BASE_SOURCES}

  ${LIVE_CURRENT_HEADERS}
  ${LIVE_CURRENT_SOURCES}
)

target_link_libraries(${PROJECT_NAME}-live PRIVATE lssvc::utils lssvc::net lssvc::mmedia)

add_library(
  lssvc::live ALIAS ${PROJECT_NAME}-live
)

set_target_properties(
  ${PROJECT_NAME}-live PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  EXPORT_NAME live
)
