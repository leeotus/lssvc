include_directories(${LSSVC_INCLUDE_PATH}/utils)
include_directories(${LSSVC_INCLUDE_PATH}/network)
include_directories(${LSSVC_INCLUDE_PATH}/mmedia)

add_executable("test_rtmp_handshake_server" rtmp/test_rtmp_handshake_server.cpp)
add_executable("test_rtmp_handshake_client" rtmp/test_rtmp_handshake_client.cpp)
add_executable("test_rtmp_server" rtmp/test_rtmp_server.cpp)
add_executable("test_rtmp_client" rtmp/test_rtmp_client.cpp)
add_executable("test_http_client" http/test_http_client.cpp)

target_link_libraries(
  "test_rtmp_handshake_server" PRIVATE lssvc::utils lssvc::net lssvc::mmedia
)

target_link_libraries(
  "test_rtmp_handshake_client" PRIVATE lssvc::utils lssvc::net lssvc::mmedia
)

target_link_libraries(
  "test_rtmp_server" PRIVATE lssvc::utils lssvc::net lssvc::mmedia
)

target_link_libraries(
  "test_rtmp_client" PRIVATE lssvc::utils lssvc::net lssvc::mmedia
)

target_link_libraries(
  "test_http_client" PRIVATE lssvc::utils lssvc::net lssvc::mmedia
)
