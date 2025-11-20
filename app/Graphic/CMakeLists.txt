set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(graphic STATIC
    graphic.cpp
    graphic.h
    d3dx12.h
    types.h
    # Pch.h
)

set_property(TARGET graphic PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_compile_options(graphic PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

target_compile_definitions(graphic PRIVATE UNICODE _UNICODE)

target_compile_definitions(graphic PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:_DEBUG>
)

# target_precompile_headers(graphic PRIVATE Pch.h)

target_include_directories(graphic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
