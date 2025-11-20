add_library(graphic STATIC
    graphic.cpp
    graphic.h
    d3dx12.h
    types.h
)

set_msvc_runtime(graphic)

target_include_directories(graphic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
