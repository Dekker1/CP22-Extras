cmake_minimum_required(VERSION 3.5)

project(blackboxes LANGUAGES CXX)

add_library(gini_val SHARED gini.cpp)
add_library(spread_val SHARED spread.cpp)

install(
				TARGETS gini_val spread_val
				RUNTIME DESTINATION bin
				LIBRARY DESTINATION lib
				ARCHIVE DESTINATION lib
)
