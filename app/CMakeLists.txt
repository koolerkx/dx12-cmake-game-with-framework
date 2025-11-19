cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(app WIN32
        main.cpp
        Application/Application.h
        Application/Application.cpp
        Application/timer_updater.h
        Application/timer_updater.cpp
)

set_property(TARGET app PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_options(app PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

target_compile_definitions(app PRIVATE UNICODE _UNICODE)

if (WIN7_SYSTEM)
    target_compile_definitions(app PRIVATE _WIN32_WINNT=0x0601)
endif ()

target_link_libraries(app PRIVATE
        d3d12.lib
        dxgi.lib
        d3dcompiler.lib
        uuid.lib
)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
