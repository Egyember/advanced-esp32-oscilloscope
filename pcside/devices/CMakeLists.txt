cmake_minimum_required(VERSION 3.16)
add_library(devices
	devices.c
	include/devices.h
	include/espsiteTypes.h
)
target_include_directories(addrlist PUBLIC
	"${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(devices PRIVATE
	addrlist
	pthread
)
