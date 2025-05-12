cmake_minimum_required(VERSION 3.16)
add_library(helper
	helper.cpp
	include/helpertypes.h
)
target_include_directories(helper PUBLIC
	"${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(helper PRIVATE
	pthread
)
