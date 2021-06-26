cmake_minimum_required(VERSION 3.1)
find_package(GTest REQUIRED)
add_executable(tester basicTest.cpp)
target_link_libraries(tester PRIVATE GTest::GTest GTest::Main)
