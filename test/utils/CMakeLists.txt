include_directories(${LSSVC_INCLUDE_PATH}/utils)

add_executable("test_lssvctime" lssvc_time_test.cpp)
add_executable("test_singleton" lssvc_singleton_test.cpp)
add_executable("test_lssvc_config" test_lssvc_config.cpp)

target_link_libraries(
  "test_lssvctime" PRIVATE lssvc::utils
)
target_link_libraries(
  "test_singleton" PRIVATE lssvc::utils
)
target_link_libraries(
  "test_lssvc_config" PRIVATE lssvc::utils
)
