add_executable("test_jsoncpp" test_jsoncpp.cpp)

target_link_directories("test_jsoncpp" PRIVATE ${CMAKE_BINARY_DIR}/third_party/jsoncpp/src/lib_json)
target_link_libraries("test_jsoncpp" PRIVATE jsoncpp)
