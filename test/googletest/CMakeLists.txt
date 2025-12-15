add_executable("test_googletest" simple_test.cpp)

target_link_directories("test_googletest" PRIVATE ${CMAKE_BINARY_DIR}/lib)
target_link_libraries("test_googletest" PRIVATE gtest gmock gtest_main gmock_main)
