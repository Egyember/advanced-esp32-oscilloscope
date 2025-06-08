cmake_minimum_required(VERSION 3.16)
add_library(recorder
	recorder.cpp
	include/recorder.h
)
target_include_directories(recorder PUBLIC
	"${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(recorder PUBLIC
	devices
	samples
	helper

)
