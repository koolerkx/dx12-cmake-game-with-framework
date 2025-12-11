include(shader)

add_library(graphic STATIC
    ThirdParty/WICTextureLoader12.h
    ThirdParty/WICTextureLoader12.cpp
    ThirdParty/d3dx12.h
    graphic.cpp
    graphic.h
    types.h
    vertex_types.h
    vertex_types.cpp
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
    render_target.h
    render_target.cpp
    fence_manager.h
    fence_manager.cpp

    texture.h
    texture.cpp
    buffer.h
    buffer.cpp
    upload_context.h
    upload_context.cpp

    pipeline_state_builder.h
    pipeline_state_builder.cpp
    root_signature_builder.h
    root_signature_builder.cpp
    shader_manager.h
    shader_manager.cpp

    texture_manager.h
    texture_manager.cpp

    Material/material_template.h
    Material/material_template.cpp
    Material/material_instance.h
    Material/material_instance.cpp
    Material/material_manager.h
    Material/material_manager.cpp

    primitive_geometry_2d.h
    primitive_geometry_2d.cpp

    mesh.h
    mesh.cpp
    RenderPass/scene_renderer.h
    RenderPass/scene_renderer.cpp
    RenderPass/render_layer.h
    RenderPass/render_pass_manager.h
    RenderPass/render_pass_manager.cpp
    RenderPass/fullscreen_pass_helper.h
    RenderPass/fullscreen_pass_helper.cpp

    RenderPass/render_pass.h
    RenderPass/forward_pass.h
    RenderPass/forward_pass.cpp
    RenderPass/ui_pass.h
    RenderPass/ui_pass.cpp
    RenderPass/depth_prepass.h
    RenderPass/depth_prepass.cpp
    RenderPass/postprocess_pass.h
    RenderPass/postprocess_pass.cpp

    # Framework default assets
    framework_default_assets.h
    framework_default_assets.cpp
    
    # Debug visual system
    debug_visual_service.h
    debug_visual_service.cpp
    debug_visual_renderer.h
    debug_visual_renderer.cpp
)

target_include_directories(graphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(graphic SYSTEM PRIVATE ThirdParty)
target_include_directories(graphic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(graphic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Material)

set_msvc_runtime(graphic)

target_add_hlsl_auto(graphic "6.5"
    "${CMAKE_SOURCE_DIR}/shaders/basic.vs.hlsl"
    "${CMAKE_SOURCE_DIR}/shaders/basic.ps.hlsl"
    "${CMAKE_SOURCE_DIR}/shaders/debug_line.vs.hlsl"
    "${CMAKE_SOURCE_DIR}/shaders/debug_line.ps.hlsl"
)

# manually add shaders files
# target_add_hlsl(graphic "5.0" "vs"
#     "${CMAKE_SOURCE_DIR}/shaders/basic_vs.hlsl"
# )
# target_add_hlsl(graphic "5.0" "Pixel"
#     "${CMAKE_SOURCE_DIR}/shaders/basic_ps.hlsl"
# )

