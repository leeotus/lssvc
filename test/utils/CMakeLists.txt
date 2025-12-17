include_directories(${LSSVC_INCLUDE_PATH}/utils)

add_executable("test_lssvctime" test_lssvc_time.cpp)
add_executable("test_singleton" test_lssvc_singleton.cpp)
add_executable("test_lssvc_logger" test_lssvc_logger.cpp)

target_link_libraries(
  "test_lssvctime" PRIVATE lssvc::utils
)
target_link_libraries(
  "test_singleton" PRIVATE lssvc::utils
)

target_link_libraries(
  "test_lssvc_logger" PRIVATE lssvc::utils
)
