cmake_minimum_required(VERSION 3.16)
add_library(devices
	devices.cpp
	ringBuffer.cpp
	include/devices.h
	include/espsiteTypes.h
	include/ringbuffer.h
)
target_include_directories(devices PUBLIC
	"${CMAKE_CURRENT_LIST_DIR}/include"
)
#target_compile_options(devices PRIVATE -DNDEBUG)
#target_compile_options(devices PRIVATE -DDEBUGSAMPLE)
target_compile_options(devices PRIVATE -DDEBUGRINGBUFFER)

target_link_libraries(devices PRIVATE
	samples
	helper
	addrlist
	pthread
	helper
)
