include(shader)

add_library(graphic STATIC
    WICTextureLoader12.h
    WICTextureLoader12.cpp
    graphic.cpp
    graphic.h
    d3dx12.h
    types.h
)

set_msvc_runtime(graphic)

target_add_hlsl_auto(graphic "6.5"
    "${CMAKE_SOURCE_DIR}/shaders/basic.vs.hlsl"
    "${CMAKE_SOURCE_DIR}/shaders/basic.ps.hlsl"
)

# manually add shaders files
# target_add_hlsl(graphic "5.0" "vs"
#     "${CMAKE_SOURCE_DIR}/shaders/basic_vs.hlsl"
# )
# target_add_hlsl(graphic "5.0" "Pixel"
#     "${CMAKE_SOURCE_DIR}/shaders/basic_ps.hlsl"
# )

target_include_directories(graphic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
