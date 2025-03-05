
cmake_minimum_required(VERSION 3.16)
add_library(rayguiimpl
	rayguiimpl.c
)
target_include_directories(rayguiimpl PUBLIC
	"${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(addrlist PRIVATE
	raylib
)
