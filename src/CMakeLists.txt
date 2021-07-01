cmake_minimum_required(VERSION 3.1)
project(c2pa CXX)
add_library(cparser STATIC ctoken.cpp clexer.cpp)
add_executable(c2pa c2pa.cpp)
target_link_libraries(c2pa cparser)

if (CMAKE_COMPILER_IS_GNUCXX)
	set_source_files_properties(clexer.cpp PROPERTIES COMPILE_FLAGS -Wno-multichar)
endif()
