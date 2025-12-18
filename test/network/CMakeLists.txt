include_directories(${LSSVC_INCLUDE_PATH}/utils)
include_directories(${LSSVC_INCLUDE_PATH}/network)

add_executable("test_eventloop_thread" test_eventloop_thread.cpp)

add_executable("test_timingwheels" test_timingwheels.cpp)

target_link_libraries(
  "test_eventloop_thread" PRIVATE lssvc::utils lssvc::net
)

target_link_libraries(
  "test_timingwheels" PRIVATE lssvc::utils lssvc::net
)
