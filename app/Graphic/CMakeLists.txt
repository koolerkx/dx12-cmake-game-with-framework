include(shader)

add_library(graphic STATIC
    ThirdParty/WICTextureLoader12.h
    ThirdParty/WICTextureLoader12.cpp
    ThirdParty/d3dx12.h
    graphic.cpp
    graphic.h
    types.h
    ../utils.h

    gpu_resource.h
    gpu_resource.cpp
    descriptor_heap_allocator.h
    descriptor_heap_allocator.cpp
    descriptor_heap_manager.h
    descriptor_heap_manager.cpp

    swapchain_manager.h
    swapchain_manager.cpp
    depth_buffer.h
    depth_buffer.cpp
    fence_manager.h
    fence_manager.cpp

    texture.h
    texture.cpp
    buffer.h
    buffer.cpp

    pipeline_state_builder.h
    pipeline_state_builder.cpp
    root_signature_builder.h
    root_signature_builder.cpp
    shader_manager.h
    shader_manager.cpp

    texture_manager.h
    texture_manager.cpp
)

target_include_directories(graphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(graphic SYSTEM PRIVATE ThirdParty)
target_include_directories(graphic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

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

