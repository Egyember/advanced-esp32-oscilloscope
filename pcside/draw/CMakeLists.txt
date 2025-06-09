cmake_minimum_required(VERSION 3.16)
add_library(draw
	devices.c
	graph.cpp
	include/drawDevices.h
	include/drawGraph.h
)
target_include_directories(draw PUBLIC
	"${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(draw PUBLIC
	rayguiimpl
	devices
	addrlist
	pthread
)
