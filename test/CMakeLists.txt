cmake_minimum_required(VERSION 3.1)

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(tester basicTest.cpp)
target_link_libraries(tester cparser GTest::GTest GTest::Main)

if (CMAKE_COMPILER_IS_GNUCXX)
	 if (OUTPUT_COVERAGE)
		set_target_properties(tester PROPERTIES LINK_FLAGS "--coverage")
	endif()
endif()

# Copy test case files.
file(GLOB CASE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "cases/*.c")
foreach(CASE_FILE IN LISTS CASE_FILES)
	configure_file(${CASE_FILE} ${CASE_FILE} COPYONLY)
endforeach()

gtest_discover_tests(tester)

