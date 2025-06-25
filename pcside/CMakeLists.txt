cmake_minimum_required(VERSION 3.31)
set(CMAKE_C++_COMPILER "/usr/bin/g++")
project(
	espScope
	VERSION 0.0.1
	LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "") # hack to make it work
#add_compile_options( -Wall -O2 --std=c++17 -g )
add_compile_options( -Wall -O0 --std=c++17 -g -fstack-protector-strong)
add_executable(main main.cpp guifuncs.cpp)
add_executable(test test.cpp)
target_include_directories(main PUBLIC
	"${PROJECT_SOURCE_DIR}/include"
)
target_include_directories(test PUBLIC
	"${PROJECT_SOURCE_DIR}/include"
)
add_subdirectory(helper)
add_subdirectory(sample)
add_subdirectory(recorder)
add_subdirectory(addrlist)
add_subdirectory(rayguiimpl)
add_subdirectory(devices)
add_subdirectory(draw)
target_link_libraries(main PRIVATE
	helper
	samples
	devices
	recorder
	pthread
	raylib
	rayguiimpl
	addrlist
	draw
	m
)

target_link_libraries(test PRIVATE
	samples
	helper
	pthread
	raylib
	rayguiimpl
	addrlist
	devices
	draw
	m
)
