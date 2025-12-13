include_directories(${LSSVC_INCLUDE_PATH}/utils)

file(GLOB UTILS_HEADERS "${LSSVC_INCLUDE_PATH}/utils/**.h")
file(GLOB UTILS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/**.cpp")

add_library(
  utils
  ${UTILS_HEADERS}
  ${UTILS_SOURCES}
)

add_library(
  lssvc::utils ALIAS utils
)

set_target_properties(
  utils PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  EXPORT_NAME utils
)

