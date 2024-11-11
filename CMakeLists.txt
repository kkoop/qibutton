cmake_minimum_required(VERSION 3.5)
project(qibutton)

message(STATUS "output in ${CMAKE_SOURCE_DIR}/bin")

add_subdirectory(cli)
add_subdirectory(gui)