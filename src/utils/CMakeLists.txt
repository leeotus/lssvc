include_directories(${LSSVC_INCLUDE_PATH}/utils)

file(GLOB UTILS_HEADERS "${LSSVC_INCLUDE_PATH}/utils/**.h")
file(GLOB UTILS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/**.cpp")

add_library(
  ${PROJECT_NAME}-utils
  ${UTILS_HEADERS}
  ${UTILS_SOURCES}
)

# target_link_directories(utils PRIVATE ${CMAKE_BINARY_DIR}/third_party/jsoncpp/src/lib_json)
target_link_libraries(utils PRIVATE jsoncpp)

add_library(
  lssvc::utils ALIAS ${PROJECT_NAME}-utils
)

set_target_properties(
  ${PROJECT_NAME}-utils PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  EXPORT_NAME utils
)

