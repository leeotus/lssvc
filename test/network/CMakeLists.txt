include_directories(${LSSVC_INCLUDE_PATH}/utils)
include_directories(${LSSVC_INCLUDE_PATH}/network)

add_executable("test_eventloop_thread" test_eventloop_thread.cpp)

target_link_libraries(
  "test_eventloop_thread" PRIVATE lssvc::utils lssvc::net
)
