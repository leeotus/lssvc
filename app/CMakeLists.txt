include_directories(${LSSVC_INCLUDE_PATH}/utils)
include_directories(${LSSVC_INCLUDE_PATH}/network)
include_directories(${LSSVC_INCLUDE_PATH}/mmedia)
include_directories(${LSSVC_INCLUDE_PATH}/live)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(
  ${PROJECT_NAME} PRIVATE
  lssvc::utils
  lssvc::net
  lssvc::mmedia
  lssvc::live
)

