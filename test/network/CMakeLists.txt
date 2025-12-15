include_directories(${LSSVC_INCLUE_PATH}/utils)

add_executable("test_eventloop_thread" test_eventloop_thread.cpp)

target_link_libraries(
  "test_eventloop_thread" PRIVATE lssvc::utils lssvc::net
)
