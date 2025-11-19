cmake_minimum_required(VERSION 3.15)

add_executable(app WIN32 "main.cpp")

set_property(TARGET app PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_options(app PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

add_compile_definitions(UNICODE _UNICODE)
if(${WIN7_SYSTEM})
    target_compile_definitions(${TARGET_NAME} PRIVATE _WIN32_WINNT=0x0601)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
