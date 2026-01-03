include_directories(${LSSVC_INCLUDE_PATH}/utils)
include_directories(${LSSVC_INCLUDE_PATH}/network)

add_executable("test_eventloop_thread" test_eventloop_thread.cpp)
add_executable("test_timingwheels" test_timingwheels.cpp)
add_executable("test_inetaddress" test_inetaddress.cpp)
add_executable("test_acceptor" test_acceptor.cpp)
add_executable("test_tcpconnection" test_tcpconnection.cpp)
add_executable("test_tcpserver" test_tcpserver.cpp)
add_executable("test_tcpclient" test_tcpclient.cpp)
add_executable("test_dns_service" test_dns_service.cpp)

target_link_libraries(
  "test_eventloop_thread" PRIVATE lssvc::utils lssvc::net
)

target_link_libraries(
  "test_timingwheels" PRIVATE lssvc::utils lssvc::net
)

target_link_libraries(
  "test_inetaddress" PRIVATE lssvc::utils lssvc::net gtest gmock
)

target_link_libraries(
  "test_acceptor" PRIVATE lssvc::utils lssvc::net
)

target_link_libraries(
  "test_tcpconnection" PRIVATE lssvc::utils lssvc::net
)

target_link_libraries(
  "test_tcpserver" PRIVATE lssvc::utils lssvc::net
)

target_link_libraries(
  "test_tcpclient" PRIVATE lssvc::utils lssvc::net
)

target_link_libraries(
  "test_dns_service" PRIVATE lssvc::utils lssvc::net
)
