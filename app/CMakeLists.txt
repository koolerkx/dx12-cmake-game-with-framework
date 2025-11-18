cmake_minimum_required (VERSION 3.12)

add_executable (app WIN32 "main.cpp")

set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
