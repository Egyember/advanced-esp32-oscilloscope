cmake_minimum_required(VERSION 3.16)
add_library(samples
	samples.cpp
	include/samples.h
)
target_include_directories(devices PUBLIC
	"${CMAKE_CURRENT_LIST_DIR}/include"
)
#target_compile_options(devices PRIVATE -DNDEBUG)
target_link_libraries(samples PRIVATE
)
