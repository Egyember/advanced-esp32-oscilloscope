cmake_minimum_required(VERSION 3.16)
add_library(addrlist
	addrlist.cpp
	include/addrlist.h
)
target_include_directories(addrlist PUBLIC
	"${CMAKE_CURRENT_LIST_DIR}/include"
)

#target_compile_options(addrlist PRIVATE -DNDEBUG)
target_link_libraries(addrlist PRIVATE
	pthread
)

target_link_libraries(addrlist PUBLIC
	helper
)
