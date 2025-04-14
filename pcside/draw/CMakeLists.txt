cmake_minimum_required(VERSION 3.16)
add_library(draw
	devices.c
	include/drawDevices.h
)
target_include_directories(draw PUBLIC
	"${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(draw PRIVATE
	rayguiimpl
	devices
	addrlist
	pthread
)
