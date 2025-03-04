cmake_minimum_required(VERSION 3.31)
set(CMAKE_C_COMPILER "gcc")
project(
	espScope
	VERSION 0.0.1
	LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "") # hack to make it work
add_compile_options( -Wall -O2 --std=gnu17 -g -pthread)
add_executable(main main.c)
target_include_directories(main PUBLIC
	"${PROJECT_SOURCE_DIR}/include"
)
add_subdirectory(addrlist)
target_link_libraries(main PRIVATE
	pthread
	raylib
	addrlist
)
