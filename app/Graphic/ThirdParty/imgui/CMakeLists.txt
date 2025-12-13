cmake_minimum_required(VERSION 3.15)

add_library(imgui STATIC)

target_sources(imgui PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/imgui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui_draw.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui_widgets.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui_tables.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui_demo.cpp

    ${CMAKE_CURRENT_LIST_DIR}/imgui_impl_win32.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui_impl_dx12.cpp

    ${CMAKE_CURRENT_LIST_DIR}/imgui_stdlib.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_features(imgui PUBLIC cxx_std_17)
