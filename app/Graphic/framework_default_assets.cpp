#include "framework_default_assets.h"

#include <cmath>
#include <cstdint>
#include <vector>

#include "buffer.h"
#include "graphic.h"
#include "material_instance.h"
#include "material_manager.h"
#include "material_template.h"
#include "mesh.h"
#include "pipeline_state_builder.h"
#include "primitive_geometry_2d.h"
#include "root_signature_builder.h"
#include "shader_manager.h"
#include "texture_manager.h"
#include "upload_context.h"
#include "vertex_types.h"

#include "Framework/Logging/logger.h"

namespace {
constexpr const char* kSpriteWorldOpaqueInstance = "SpriteWorldOpaque_Default";
constexpr const char* kSpriteWorldTransparentInstance = "SpriteWorldTransparent_Default";
constexpr const char* kSpriteUIInstance = "SpriteUI_Default";
constexpr const char* kDebugLineOverlayInstance = "DebugLine_Overlay_Default";
constexpr const char* kDebugLineDepthInstance = "DebugLine_Depth_Default";
}  // namespace

void FrameworkDefaultAssets::Initialize(Graphic& graphic) {
  graphic_ = &graphic;

  // Create a simple rect mesh using the primitive geometry helper.
  try {
    rect2d_mesh_ = graphic.GetPrimitiveGeometry2D().CreateRect(graphic.GetUploadContext());
  } catch (...) {
    rect2d_mesh_ = nullptr;
  }

  // Create primitive 3D meshes (cube and cylinder)
  try {
    cube_mesh_ = CreateCubeMesh(graphic.GetDevice(), graphic.GetUploadContext());
  } catch (...) {
    cube_mesh_ = nullptr;
  }
  
  try {
    cylinder_mesh_ = CreateCylinderMesh(graphic.GetDevice(), graphic.GetUploadContext());
  } catch (...) {
    cylinder_mesh_ = nullptr;
  }

  // Create sphere and capsule meshes
  try {
    sphere_mesh_ = CreateSphereMesh(graphic.GetDevice(), graphic.GetUploadContext());
  } catch (...) {
    sphere_mesh_ = nullptr;
  }
  
  try {
    capsule_mesh_ = CreateCapsuleMesh(graphic.GetDevice(), graphic.GetUploadContext());
  } catch (...) {
    capsule_mesh_ = nullptr;
  }

  // Create default textures in a single upload batch using the upload context
  graphic.ExecuteImmediate([&](ID3D12GraphicsCommandList* cmd) {
    auto& tex_mgr = graphic.GetTextureManager();

    // 1x1 white
    uint8_t white_px[4] = {255, 255, 255, 255};
    white_texture_ = tex_mgr.CreateTextureFromMemory(cmd, white_px, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, "Default_White");

    // 1x1 black
    uint8_t black_px[4] = {0, 0, 0, 255};
    black_texture_ = tex_mgr.CreateTextureFromMemory(cmd, black_px, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, "Default_Black");

    // 1x1 flat normal (128,128,255,255)
    uint8_t flat_normal_px[4] = {128, 128, 255, 255};
    flat_normal_texture_ = tex_mgr.CreateTextureFromMemory(cmd, flat_normal_px, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, "Default_FlatNormal");

    // 8x8 error checker (pink and black)
    const int checker_w = 8;
    const int checker_h = 8;
    std::vector<uint8_t> checker_data(checker_w * checker_h * 4);
    for (int y = 0; y < checker_h; ++y) {
      for (int x = 0; x < checker_w; ++x) {
        int idx = (y * checker_w + x) * 4;
        bool pink = ((x + y) % 2) == 0;
        if (pink) {
          checker_data[idx + 0] = 255;  // R
          checker_data[idx + 1] = 0;    // G
          checker_data[idx + 2] = 255;  // B
          checker_data[idx + 3] = 255;  // A
        } else {
          checker_data[idx + 0] = 0;
          checker_data[idx + 1] = 0;
          checker_data[idx + 2] = 0;
          checker_data[idx + 3] = 255;
        }
      }
    }

    error_texture_ =
      tex_mgr.CreateTextureFromMemory(cmd, checker_data.data(), checker_w, checker_h, DXGI_FORMAT_R8G8B8A8_UNORM, "Default_ErrorChecker");
  });

  // Create default materials after textures are available
  CreateDefaultMaterials(graphic);
}

void FrameworkDefaultAssets::Shutdown() {
  // Release textures and material instances through their managers (Graphic still owns managers at this point)
  if (graphic_) {
    auto& tex_mgr = graphic_->GetTextureManager();
    if (tex_mgr.IsValid(white_texture_)) tex_mgr.ReleaseTexture(white_texture_);
    if (tex_mgr.IsValid(black_texture_)) tex_mgr.ReleaseTexture(black_texture_);
    if (tex_mgr.IsValid(flat_normal_texture_)) tex_mgr.ReleaseTexture(flat_normal_texture_);
    if (tex_mgr.IsValid(error_texture_)) tex_mgr.ReleaseTexture(error_texture_);

    auto& material_mgr = graphic_->GetMaterialManager();
    material_mgr.RemoveInstance(kSpriteWorldOpaqueInstance);
    material_mgr.RemoveInstance(kSpriteWorldTransparentInstance);
    material_mgr.RemoveInstance(kSpriteUIInstance);
    material_mgr.RemoveInstance(kDebugLineOverlayInstance);
    material_mgr.RemoveInstance(kDebugLineDepthInstance);
  }

  // Reset all handles and pointers
  white_texture_ = INVALID_TEXTURE_HANDLE;
  black_texture_ = INVALID_TEXTURE_HANDLE;
  flat_normal_texture_ = INVALID_TEXTURE_HANDLE;
  error_texture_ = INVALID_TEXTURE_HANDLE;

  graphic_ = nullptr;
  rect2d_mesh_.reset();
  cube_mesh_.reset();
  cylinder_mesh_.reset();
  sphere_mesh_.reset();
  capsule_mesh_.reset();
  sprite_world_opaque_template_ = nullptr;
  sprite_world_transparent_template_ = nullptr;
  sprite_ui_template_ = nullptr;
  debug_line_template_overlay_ = nullptr;
  debug_line_template_depth_ = nullptr;
  debug_line_template_depth_biased_ = nullptr;
  sprite_world_opaque_material_ = nullptr;
  sprite_world_transparent_material_ = nullptr;
  sprite_ui_material_ = nullptr;
  debug_line_material_overlay_ = nullptr;
  debug_line_material_depth_ = nullptr;
  debug_line_material_depth_biased_ = nullptr;
}

TextureHandle FrameworkDefaultAssets::GetWhiteTexture() const {
  return white_texture_;
}

TextureHandle FrameworkDefaultAssets::GetBlackTexture() const {
  return black_texture_;
}

TextureHandle FrameworkDefaultAssets::GetFlatNormalTexture() const {
  return flat_normal_texture_;
}

TextureHandle FrameworkDefaultAssets::GetErrorTexture() const {
  return error_texture_;
}

std::shared_ptr<Mesh> FrameworkDefaultAssets::GetRect2DMesh() const {
  return rect2d_mesh_;
}

MaterialInstance* FrameworkDefaultAssets::GetSprite2DDefaultMaterial() const {
  return sprite_ui_material_;
}

MaterialInstance* FrameworkDefaultAssets::GetSpriteWorldOpaqueMaterial() const {
  return sprite_world_opaque_material_;
}

MaterialInstance* FrameworkDefaultAssets::GetSpriteWorldTransparentMaterial() const {
  return sprite_world_transparent_material_;
}

MaterialInstance* FrameworkDefaultAssets::GetSpriteUIMaterial() const {
  return sprite_ui_material_;
}

MaterialTemplate* FrameworkDefaultAssets::GetDebugLineTemplateOverlay() const {
  return debug_line_template_overlay_;
}

MaterialTemplate* FrameworkDefaultAssets::GetDebugLineTemplateDepth() const {
  return debug_line_template_depth_;
}

MaterialInstance* FrameworkDefaultAssets::GetDebugLineMaterialOverlay() const {
  return debug_line_material_overlay_;
}

MaterialInstance* FrameworkDefaultAssets::GetDebugLineMaterialDepth() const {
  return debug_line_material_depth_;
}

MaterialTemplate* FrameworkDefaultAssets::GetDebugLineTemplateDepthBiased() const {
  return debug_line_template_depth_biased_;
}

MaterialInstance* FrameworkDefaultAssets::GetDebugLineMaterialDepthBiased() const {
  return debug_line_material_depth_biased_;
}

void FrameworkDefaultAssets::CreateDefaultMaterials(Graphic& gfx) {
  auto& shader_mgr = gfx.GetShaderManager();

  // Ensure shaders are loaded
  if (!shader_mgr.HasShader("BasicVS")) {
    if (!shader_mgr.LoadShader(L"Content/shaders/basic.vs.cso", ShaderType::Vertex, "BasicVS")) {
      // Fallback to loaded shader or log error
        Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to load BasicVS shader");
      return;
    }
  }

  if (!shader_mgr.HasShader("BasicPS")) {
    if (!shader_mgr.LoadShader(L"Content/shaders/basic.ps.cso", ShaderType::Pixel, "BasicPS")) {
        Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to load BasicPS shader");
      return;
    }
  }

  if (!shader_mgr.HasShader("DebugLineVS")) {
    if (!shader_mgr.LoadShader(L"Content/shaders/debug_line.vs.cso", ShaderType::Vertex, "DebugLineVS")) {
        Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to load DebugLineVS shader");
      return;
    }
  }

  if (!shader_mgr.HasShader("DebugLinePS")) {
    if (!shader_mgr.LoadShader(L"Content/shaders/debug_line.ps.cso", ShaderType::Pixel, "DebugLinePS")) {
        Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to load DebugLinePS shader");
      return;
    }
  }

  // Create Sprite2D material
  CreateSpriteMaterials(gfx);

  // Create DebugLine materials (overlay and depth-tested)
  CreateDebugLineMaterials(gfx);
}

void FrameworkDefaultAssets::CreateSpriteMaterials(Graphic& gfx) {
  auto& shader_mgr = gfx.GetShaderManager();
  auto& material_mgr = gfx.GetMaterialManager();

  // Create root signature shared across sprite variants
  ComPtr<ID3D12RootSignature> sprite_root_signature;
  RootSignatureBuilder rs_builder;
  rs_builder
    .AddRootConstant(16, 0, D3D12_SHADER_VISIBILITY_VERTEX)                                    // b0 - Object constants
    .AddRootConstant(4, 2, D3D12_SHADER_VISIBILITY_VERTEX)                                     // b2 - Per-object color tint
    .AddRootConstant(4, 3, D3D12_SHADER_VISIBILITY_VERTEX)                                     // b3 - Per-object UV transform
    .AddRootCBV(1, D3D12_SHADER_VISIBILITY_ALL)                                                // b1 - Frame CB
    .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL)  // t0 - Texture
    .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL)
    .AllowInputLayout();

  if (!rs_builder.Build(gfx.GetDevice(), sprite_root_signature)) {
     Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to create Sprite2D root signature");
    return;
  }

  // Common shader blobs and input layout
  const ShaderBlob* vs = shader_mgr.GetShader("BasicVS");
  const ShaderBlob* ps = shader_mgr.GetShader("BasicPS");
  auto input_layout = GetInputLayout_VertexPositionTexture2D();

  std::vector<TextureSlotDefinition> sprite_texture_slots = {
    {"BaseColor", 4, D3D12_SHADER_VISIBILITY_PIXEL}  // t0, parameter index 4 (descriptor table)
  };

  // World Opaque (depth write)
  {
    ComPtr<ID3D12PipelineState> sprite_pso;
    PipelineStateBuilder pso_builder;
    pso_builder.SetVertexShader(vs)
      .SetPixelShader(ps)
      .SetInputLayout(input_layout.data(), static_cast<UINT>(input_layout.size()))
      .SetRootSignature(sprite_root_signature.Get())
      .SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
      .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
      .SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT)
      .SetDepthEnable(true)
      .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ALL)
      .SetDepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL)
      .SetBlendEnable(false, 0);

    if (pso_builder.Build(gfx.GetDevice(), sprite_pso)) {
      sprite_world_opaque_template_ =
        material_mgr.CreateTemplate("SpriteWorldOpaque", sprite_pso.Get(), sprite_root_signature.Get(), sprite_texture_slots);

      if (sprite_world_opaque_template_) {
        sprite_world_opaque_material_ = material_mgr.CreateInstance(kSpriteWorldOpaqueInstance, sprite_world_opaque_template_);
        if (sprite_world_opaque_material_) {
          sprite_world_opaque_material_->SetTexture("BaseColor", white_texture_);
           Logger::Log(LogLevel::Info, LogCategory::Graphic, "[FrameworkDefaultAssets] Created SpriteWorldOpaque material");
        }
      }
    } else {
        Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to create SpriteWorldOpaque PSO");
    }
  }

  // World Transparent (depth read, no write)
  {
    ComPtr<ID3D12PipelineState> sprite_pso;
    PipelineStateBuilder pso_builder;
    pso_builder.SetVertexShader(vs)
      .SetPixelShader(ps)
      .SetInputLayout(input_layout.data(), static_cast<UINT>(input_layout.size()))
      .SetRootSignature(sprite_root_signature.Get())
      .SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
      .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
      .SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT)
      .SetDepthEnable(true)
      .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO)
      .SetDepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL)
      .SetBlendEnable(true, 0)
      .SetBlendFactors(D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, 0);

    if (pso_builder.Build(gfx.GetDevice(), sprite_pso)) {
      sprite_world_transparent_template_ =
        material_mgr.CreateTemplate("SpriteWorldTransparent", sprite_pso.Get(), sprite_root_signature.Get(), sprite_texture_slots);

      if (sprite_world_transparent_template_) {
        sprite_world_transparent_material_ =
          material_mgr.CreateInstance(kSpriteWorldTransparentInstance, sprite_world_transparent_template_);
        if (sprite_world_transparent_material_) {
          sprite_world_transparent_material_->SetTexture("BaseColor", white_texture_);
           Logger::Log(LogLevel::Info, LogCategory::Graphic, "[FrameworkDefaultAssets] Created SpriteWorldTransparent material");
        }
      }
    } else {
        Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to create SpriteWorldTransparent PSO");
    }
  }

  // UI (no depth)
  {
    ComPtr<ID3D12PipelineState> sprite_pso;
    PipelineStateBuilder pso_builder;
    pso_builder.SetVertexShader(vs)
      .SetPixelShader(ps)
      .SetInputLayout(input_layout.data(), static_cast<UINT>(input_layout.size()))
      .SetRootSignature(sprite_root_signature.Get())
      .SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
      .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
      .SetDepthStencilFormat(DXGI_FORMAT_UNKNOWN)
      .SetDepthEnable(false)
      .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO)
      // UI uses an ortho projection with negative Y scale (top-left origin),
      // which flips winding. Disable culling to avoid backface-culling the quad.
      .SetCullMode(D3D12_CULL_MODE_NONE)
      .SetBlendEnable(true, 0)
      .SetBlendFactors(D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, 0);

    if (pso_builder.Build(gfx.GetDevice(), sprite_pso)) {
      sprite_ui_template_ = material_mgr.CreateTemplate("SpriteUI", sprite_pso.Get(), sprite_root_signature.Get(), sprite_texture_slots);

      if (sprite_ui_template_) {
        sprite_ui_material_ = material_mgr.CreateInstance(kSpriteUIInstance, sprite_ui_template_);
        if (sprite_ui_material_) {
          sprite_ui_material_->SetTexture("BaseColor", white_texture_);
           Logger::Log(LogLevel::Info, LogCategory::Graphic, "[FrameworkDefaultAssets] Created SpriteUI material");
        }
      }
    } else {
        Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to create SpriteUI PSO");
    }
  }
}

void FrameworkDefaultAssets::CreateDebugLineMaterials(Graphic& gfx) {
  auto& shader_mgr = gfx.GetShaderManager();
  auto& material_mgr = gfx.GetMaterialManager();

  // Shared root signature for both overlay and depth-tested debug lines
  // Updated for instancing: removed b0 world matrix (now comes from instance buffer)
  ComPtr<ID3D12RootSignature> debug_root_signature;
  RootSignatureBuilder rs_builder;
  rs_builder
    .AddRootCBV(1, D3D12_SHADER_VISIBILITY_VERTEX)  // b1 - FrameCB (view/proj)
    .AllowInputLayout();

  if (!rs_builder.Build(gfx.GetDevice(), debug_root_signature)) {
     Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to create DebugLine root signature");
    return;
  }

  const ShaderBlob* vs = shader_mgr.GetShader("DebugLineVS");
  const ShaderBlob* ps = shader_mgr.GetShader("DebugLinePS");
  auto input_layout = GetInputLayout_DebugLineInstanced();

  // Overlay PSO (depth disabled)
  {
    ComPtr<ID3D12PipelineState> overlay_pso;
    PipelineStateBuilder pso_builder;
    pso_builder.SetVertexShader(vs)
      .SetPixelShader(ps)
      .SetInputLayout(input_layout.data(), static_cast<UINT>(input_layout.size()))
      .SetRootSignature(debug_root_signature.Get())
      .SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE)
      .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
      .SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT)
      .SetDepthEnable(false)
      .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO)
      .SetDepthFunc(D3D12_COMPARISON_FUNC_ALWAYS)
      .SetCullMode(D3D12_CULL_MODE_NONE)
      .SetFillMode(D3D12_FILL_MODE_SOLID);

    if (!pso_builder.Build(gfx.GetDevice(), overlay_pso)) {
      Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to create DebugLine overlay PSO");
      return;
    }

    debug_line_template_overlay_ =
      material_mgr.CreateTemplate("DefaultDebugLineOverlay", overlay_pso.Get(), debug_root_signature.Get());

    if (debug_line_template_overlay_) {
      debug_line_material_overlay_ = material_mgr.CreateInstance(kDebugLineOverlayInstance, debug_line_template_overlay_);
      if (debug_line_material_overlay_) {
         Logger::Log(LogLevel::Info, LogCategory::Graphic, "[FrameworkDefaultAssets] Created DebugLine overlay material");
      }
    }
  }

  // Depth-tested PSO (read-only depth)
  {
    ComPtr<ID3D12PipelineState> depth_pso;
    PipelineStateBuilder pso_builder;
    pso_builder.SetVertexShader(vs)
      .SetPixelShader(ps)
      .SetInputLayout(input_layout.data(), static_cast<UINT>(input_layout.size()))
      .SetRootSignature(debug_root_signature.Get())
      .SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE)
      .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
      .SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT)
      .SetDepthEnable(true)
      .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO)  // Read-only depth test
      .SetDepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL)
      .SetCullMode(D3D12_CULL_MODE_NONE)
      .SetFillMode(D3D12_FILL_MODE_SOLID);

    if (!pso_builder.Build(gfx.GetDevice(), depth_pso)) {
      Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to create DebugLine depth PSO");
      return;
    }

    debug_line_template_depth_ = material_mgr.CreateTemplate("DefaultDebugLineDepth", depth_pso.Get(), debug_root_signature.Get());

    if (debug_line_template_depth_) {
      debug_line_material_depth_ = material_mgr.CreateInstance(kDebugLineDepthInstance, debug_line_template_depth_);
      if (debug_line_material_depth_) {
         Logger::Log(LogLevel::Info, LogCategory::Graphic, "[FrameworkDefaultAssets] Created DebugLine depth material");
      }
    }
  }

  // Depth-tested PSO with depth bias (Task 3.2)
  // Used to prevent Z-fighting for surface-aligned debug visuals (grid, navmesh overlays)
  {
    ComPtr<ID3D12PipelineState> depth_biased_pso;
    PipelineStateBuilder pso_builder;
    pso_builder.SetVertexShader(vs)
      .SetPixelShader(ps)
      .SetInputLayout(input_layout.data(), static_cast<UINT>(input_layout.size()))
      .SetRootSignature(debug_root_signature.Get())
      .SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE)
      .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
      .SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT)
      .SetDepthEnable(true)
      .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO)  // Read-only depth test
      .SetDepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL)
      .SetCullMode(D3D12_CULL_MODE_NONE)
      .SetFillMode(D3D12_FILL_MODE_SOLID);

    // Set depth bias to prevent Z-fighting
    // Default values: DepthBias = -10000, SlopeScaledDepthBias = -2.0f
    // These can be adjusted via DebugVisualSettings
    pso_builder.SetDepthBias(-10000, 0.0f, -2.0f);

    if (!pso_builder.Build(gfx.GetDevice(), depth_biased_pso)) {
      Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to create DebugLine depth-biased PSO");
      return;
    }

    debug_line_template_depth_biased_ = 
      material_mgr.CreateTemplate("DefaultDebugLineDepthBiased", depth_biased_pso.Get(), debug_root_signature.Get());

    if (debug_line_template_depth_biased_) {
      debug_line_material_depth_biased_ = 
        material_mgr.CreateInstance("DebugLine_DepthBiased_Default", debug_line_template_depth_biased_);
      if (debug_line_material_depth_biased_) {
         Logger::Log(LogLevel::Info, LogCategory::Graphic, "[FrameworkDefaultAssets] Created DebugLine depth-biased material");
      }
    }
  }
}

std::shared_ptr<Mesh> FrameworkDefaultAssets::GetCubeMesh() const {
  return cube_mesh_;
}

std::shared_ptr<Mesh> FrameworkDefaultAssets::GetCylinderMesh() const {
  return cylinder_mesh_;
}

std::shared_ptr<Mesh> FrameworkDefaultAssets::GetSphereMesh() const {
  return sphere_mesh_;
}

std::shared_ptr<Mesh> FrameworkDefaultAssets::GetCapsuleMesh() const {
  return capsule_mesh_;
}

std::shared_ptr<Mesh> FrameworkDefaultAssets::CreateCubeMesh(ID3D12Device* device, UploadContext& upload_context) {
  // Cube with 24 vertices (4 per face) and 36 indices (6 faces * 2 triangles * 3 indices)
  // Each face has unique vertices to allow proper UV mapping
  std::vector<VertexPositionTexture2D> vertices;
  std::vector<uint16_t> indices;
  
  vertices.reserve(24);
  indices.reserve(36);
  
  // Define cube half-extents
  constexpr float s = 0.5f;
  
  // Front face (+Z)
  vertices.push_back({{-s, -s, +s}, {0, 1}});  // 0: bottom-left
  vertices.push_back({{+s, -s, +s}, {1, 1}});  // 1: bottom-right
  vertices.push_back({{-s, +s, +s}, {0, 0}});  // 2: top-left
  vertices.push_back({{+s, +s, +s}, {1, 0}});  // 3: top-right
  
  // Back face (-Z)
  vertices.push_back({{+s, -s, -s}, {0, 1}});  // 4: bottom-left (viewed from back)
  vertices.push_back({{-s, -s, -s}, {1, 1}});  // 5: bottom-right
  vertices.push_back({{+s, +s, -s}, {0, 0}});  // 6: top-left
  vertices.push_back({{-s, +s, -s}, {1, 0}});  // 7: top-right
  
  // Left face (-X)
  vertices.push_back({{-s, -s, -s}, {0, 1}});  // 8: bottom-left
  vertices.push_back({{-s, -s, +s}, {1, 1}});  // 9: bottom-right
  vertices.push_back({{-s, +s, -s}, {0, 0}});  // 10: top-left
  vertices.push_back({{-s, +s, +s}, {1, 0}});  // 11: top-right
  
  // Right face (+X)
  vertices.push_back({{+s, -s, +s}, {0, 1}});  // 12: bottom-left
  vertices.push_back({{+s, -s, -s}, {1, 1}});  // 13: bottom-right
  vertices.push_back({{+s, +s, +s}, {0, 0}});  // 14: top-left
  vertices.push_back({{+s, +s, -s}, {1, 0}});  // 15: top-right
  
  // Top face (+Y)
  vertices.push_back({{-s, +s, +s}, {0, 1}});  // 16: bottom-left (front edge)
  vertices.push_back({{+s, +s, +s}, {1, 1}});  // 17: bottom-right (front edge)
  vertices.push_back({{-s, +s, -s}, {0, 0}});  // 18: top-left (back edge)
  vertices.push_back({{+s, +s, -s}, {1, 0}});  // 19: top-right (back edge)
  
  // Bottom face (-Y)
  vertices.push_back({{-s, -s, -s}, {0, 1}});  // 20: bottom-left (back edge)
  vertices.push_back({{+s, -s, -s}, {1, 1}});  // 21: bottom-right (back edge)
  vertices.push_back({{-s, -s, +s}, {0, 0}});  // 22: top-left (front edge)
  vertices.push_back({{+s, -s, +s}, {1, 0}});  // 23: top-right (front edge)
  
  // Indices (CCW winding for all faces)
  // Standard Pattern for CCW Quad (BL, BR, TL, TR):
  // Tri 1: BL(0) -> BR(1) -> TL(2)
  // Tri 2: BR(1) -> TR(3) -> TL(2)  <-- CORRECTED from {2,1,3} which was CW
  
  // Front (+Z)
  indices.insert(indices.end(), {0, 1, 2, 1, 3, 2});
  
  // Back (-Z) - Look from back: 4 is Left(+X), 5 is Right(-X)
  // To get Normal -Z, we need CCW in local view
  indices.insert(indices.end(), {4, 5, 6, 5, 7, 6});
  
  // Left (-X)
  indices.insert(indices.end(), {8, 9, 10, 9, 11, 10});
  
  // Right (+X)
  indices.insert(indices.end(), {12, 13, 14, 13, 15, 14});
  
  // Top (+Y)
  indices.insert(indices.end(), {16, 17, 18, 17, 19, 18});
  
  // Bottom (-Y)
  indices.insert(indices.end(), {20, 21, 22, 21, 23, 22});
  
  // Create vertex and index buffers
  auto vb = Buffer::CreateAndUploadToDefaultHeapForInit(
    device,
    upload_context,
    vertices.data(),
    vertices.size() * sizeof(VertexPositionTexture2D),
    Buffer::Type::Vertex,
    "Primitive_Cube_VB"
  );
  
  auto ib = Buffer::CreateAndUploadToDefaultHeapForInit(
    device,
    upload_context,
    indices.data(),
    indices.size() * sizeof(uint16_t),
    Buffer::Type::Index,
    "Primitive_Cube_IB"
  );
  
  // Create and initialize mesh
  auto mesh = std::make_shared<Mesh>();
  mesh->Initialize(
    vb,
    ib,
    sizeof(VertexPositionTexture2D),
    static_cast<uint32_t>(indices.size()),
    DXGI_FORMAT_R16_UINT,
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
  );
  mesh->SetDebugName("Primitive_Cube");
  
  return mesh;
}

std::shared_ptr<Mesh> FrameworkDefaultAssets::CreateCylinderMesh(ID3D12Device* device, UploadContext& upload_context) {
  // Cylinder with 24 segments
  constexpr int segments = 24;
  constexpr float radius = 0.5f;
  constexpr float height = 1.0f;
  constexpr float half_height = height * 0.5f;
  
  std::vector<VertexPositionTexture2D> vertices;
  std::vector<uint16_t> indices;
  
  // Reserve space
  // Side: 48 vertices (24 segments * 2 for top and bottom)
  // Top cap: 25 vertices (1 center + 24 rim)
  // Bottom cap: 25 vertices (1 center + 24 rim)
  vertices.reserve(98);
  
  // Side: 144 indices (24 segments * 6)
  // Top cap: 72 indices (24 triangles * 3)
  // Bottom cap: 72 indices (24 triangles * 3)
  indices.reserve(288);
  
  // Side vertices
  const uint16_t side_start = 0;
  for (int i = 0; i < segments; ++i) {
    float angle = (static_cast<float>(i) / segments) * DirectX::XM_2PI;
    float x = std::cos(angle) * radius;
    float z = std::sin(angle) * radius;
    float u = static_cast<float>(i) / segments;
    
    // Bottom vertex (even index)
    vertices.push_back({{x, -half_height, z}, {u, 1}});
    // Top vertex (odd index)
    vertices.push_back({{x, +half_height, z}, {u, 0}});
  }
  
  // Side indices (CORRECTED: Fix inside-out winding)
  // We want Normal Outward (radial direction)
  // Correct CCW: BottomCurr -> TopCurr -> BottomNext
  for (int i = 0; i < segments; ++i) {
    const int next_i = (i + 1) % segments;
    uint16_t b_curr = side_start + static_cast<uint16_t>(i) * 2u;
    uint16_t t_curr = b_curr + 1u;
    uint16_t b_next = side_start + static_cast<uint16_t>(next_i) * 2u;
    uint16_t t_next = b_next + 1u;
    
    // Triangle 1: BottomCurr -> TopCurr -> BottomNext
    indices.push_back(b_curr);
    indices.push_back(t_curr);
    indices.push_back(b_next);
    
    // Triangle 2: BottomNext -> TopCurr -> TopNext
    indices.push_back(b_next);
    indices.push_back(t_curr);
    indices.push_back(t_next);
  }
  
  // Top cap (Normal +Y)
  const uint16_t top_center = static_cast<uint16_t>(vertices.size());
  vertices.push_back({{0, +half_height, 0}, {0.5f, 0.5f}});  // Center
  
  for (int i = 0; i < segments; ++i) {
    float angle = (static_cast<float>(i) / segments) * DirectX::XM_2PI;
    float x = std::cos(angle) * radius;
    float z = std::sin(angle) * radius;
    float u = std::cos(angle) * 0.5f + 0.5f;
    float v = std::sin(angle) * 0.5f + 0.5f;
    
    vertices.push_back({{x, +half_height, z}, {u, v}});
  }
  
  // Top cap indices (CORRECTED: Center -> Next -> Current for +Y normal)
  for (int i = 0; i < segments; ++i) {
    const int next_i = (i + 1) % segments;
    uint16_t current = static_cast<uint16_t>(top_center + 1u + static_cast<uint16_t>(i));
    uint16_t next = static_cast<uint16_t>(top_center + 1u + static_cast<uint16_t>(next_i));
    
    indices.push_back(top_center);
    indices.push_back(next);  // Swapped for correct +Y normal
    indices.push_back(current);
  }
  
  // Bottom cap (Normal -Y)
  const uint16_t bottom_center = static_cast<uint16_t>(vertices.size());
  vertices.push_back({{0, -half_height, 0}, {0.5f, 0.5f}});  // Center
  
  for (int i = 0; i < segments; ++i) {
    float angle = (static_cast<float>(i) / segments) * DirectX::XM_2PI;
    float x = std::cos(angle) * radius;
    float z = std::sin(angle) * radius;
    float u = std::cos(angle) * 0.5f + 0.5f;
    float v = std::sin(angle) * 0.5f + 0.5f;
    
    vertices.push_back({{x, -half_height, z}, {u, v}});
  }
  
  // Bottom cap indices (Center -> Current -> Next for -Y normal)
  for (int i = 0; i < segments; ++i) {
    const int next_i = (i + 1) % segments;
    uint16_t current = static_cast<uint16_t>(bottom_center + 1u + static_cast<uint16_t>(i));
    uint16_t next = static_cast<uint16_t>(bottom_center + 1u + static_cast<uint16_t>(next_i));
    
    indices.push_back(bottom_center);
    indices.push_back(current);
    indices.push_back(next);
  }
  
  // Create vertex and index buffers
  auto vb = Buffer::CreateAndUploadToDefaultHeapForInit(
    device,
    upload_context,
    vertices.data(),
    vertices.size() * sizeof(VertexPositionTexture2D),
    Buffer::Type::Vertex,
    "Primitive_Cylinder_VB"
  );
  
  auto ib = Buffer::CreateAndUploadToDefaultHeapForInit(
    device,
    upload_context,
    indices.data(),
    indices.size() * sizeof(uint16_t),
    Buffer::Type::Index,
    "Primitive_Cylinder_IB"
  );
  
  // Create and initialize mesh
  auto mesh = std::make_shared<Mesh>();
  mesh->Initialize(
    vb,
    ib,
    sizeof(VertexPositionTexture2D),
    static_cast<uint32_t>(indices.size()),
    DXGI_FORMAT_R16_UINT,
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
  );
  mesh->SetDebugName("Primitive_Cylinder");
  
  return mesh;
}

std::shared_ptr<Mesh> FrameworkDefaultAssets::CreateSphereMesh(ID3D12Device* device, UploadContext& upload_context) {
  // Sphere with lat-long UV mapping
  constexpr int segments = 24;  // Longitude divisions
  constexpr int stacks = 16;    // Latitude divisions
  constexpr float radius = 0.5f;
  
  std::vector<VertexPositionTexture2D> vertices;
  std::vector<uint16_t> indices;
  
  // Reserve space: (segments + 1) * (stacks + 1) vertices
  vertices.reserve((segments + 1) * (stacks + 1));
  // Reserve space: segments * stacks * 6 indices (2 triangles per quad)
  indices.reserve(segments * stacks * 6);
  
  // Generate vertices
  for (int stack = 0; stack <= stacks; ++stack) {
    float phi = static_cast<float>(stack) / stacks * DirectX::XM_PI;  // 0 to PI
    float y = std::cos(phi) * radius;
    float ring_radius = std::sin(phi) * radius;
    
    for (int seg = 0; seg <= segments; ++seg) {
      float theta = static_cast<float>(seg) / segments * DirectX::XM_2PI;  // 0 to 2PI
      float x = std::cos(theta) * ring_radius;
      float z = std::sin(theta) * ring_radius;
      
      float u = static_cast<float>(seg) / segments;
      float v = static_cast<float>(stack) / stacks;
      
      vertices.push_back({{x, y, z}, {u, v}});
    }
  }
  
  // Generate indices (CCW winding)
  for (int stack = 0; stack < stacks; ++stack) {
    for (int seg = 0; seg < segments; ++seg) {
      int current = stack * (segments + 1) + seg;
      int next = current + segments + 1;
      
      // First triangle: current, current+1, next
      indices.push_back(static_cast<uint16_t>(current));
      indices.push_back(static_cast<uint16_t>(current + 1));
      indices.push_back(static_cast<uint16_t>(next));
      
      // Second triangle: current+1, next+1, next
      indices.push_back(static_cast<uint16_t>(current + 1));
      indices.push_back(static_cast<uint16_t>(next + 1));
      indices.push_back(static_cast<uint16_t>(next));
    }
  }
  
  // Create vertex and index buffers
  auto vb = Buffer::CreateAndUploadToDefaultHeapForInit(
    device,
    upload_context,
    vertices.data(),
    vertices.size() * sizeof(VertexPositionTexture2D),
    Buffer::Type::Vertex,
    "Primitive_Sphere_VB"
  );
  
  auto ib = Buffer::CreateAndUploadToDefaultHeapForInit(
    device,
    upload_context,
    indices.data(),
    indices.size() * sizeof(uint16_t),
    Buffer::Type::Index,
    "Primitive_Sphere_IB"
  );
  
  // Create and initialize mesh
  auto mesh = std::make_shared<Mesh>();
  mesh->Initialize(
    vb,
    ib,
    sizeof(VertexPositionTexture2D),
    static_cast<uint32_t>(indices.size()),
    DXGI_FORMAT_R16_UINT,
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
  );
  mesh->SetDebugName("Primitive_Sphere");
  
  return mesh;
}

std::shared_ptr<Mesh> FrameworkDefaultAssets::CreateCapsuleMesh(ID3D12Device* device, UploadContext& upload_context) {
  // Capsule = Cylinder + two hemispheres (top and bottom)
  constexpr int segments = 24;     // Circumference divisions
  constexpr int hemisphere_stacks = 8;  // Vertical divisions for each hemisphere
  constexpr float radius = 0.5f;
  constexpr float cylinder_height = 0.5f;  // Height of cylindrical portion
  constexpr float half_cyl_height = cylinder_height * 0.5f;
  
  std::vector<VertexPositionTexture2D> vertices;
  std::vector<uint16_t> indices;
  
  // Estimate vertex count: cylinder (2*segments) + 2 hemispheres (segments * (hemisphere_stacks+1) * 2)
  vertices.reserve(2 * segments + 2 * segments * (hemisphere_stacks + 1));
  indices.reserve(segments * 6 + 2 * segments * hemisphere_stacks * 6);
  
  // 1. Cylinder middle section vertices
  const uint16_t cyl_start = 0;
  for (int i = 0; i < segments; ++i) {
    float angle = (static_cast<float>(i) / segments) * DirectX::XM_2PI;
    float x = std::cos(angle) * radius;
    float z = std::sin(angle) * radius;
    float u = static_cast<float>(i) / segments;
    
    // Bottom of cylinder (at top of bottom hemisphere) - UV V = 0.25 (connects to bottom hemi equator)
    vertices.push_back({{x, -half_cyl_height, z}, {u, 0.25f}});
    // Top of cylinder (at bottom of top hemisphere) - UV V = 0.75 (connects to top hemi equator)
    vertices.push_back({{x, +half_cyl_height, z}, {u, 0.75f}});
  }
  
  // Cylinder indices (same as regular cylinder side)
  for (int i = 0; i < segments; ++i) {
    const int next_i = (i + 1) % segments;
    uint16_t b_curr = cyl_start + static_cast<uint16_t>(i) * 2u;
    uint16_t t_curr = b_curr + 1u;
    uint16_t b_next = cyl_start + static_cast<uint16_t>(next_i) * 2u;
    uint16_t t_next = b_next + 1u;
    
    indices.push_back(b_curr);
    indices.push_back(t_curr);
    indices.push_back(b_next);
    
    indices.push_back(b_next);
    indices.push_back(t_curr);
    indices.push_back(t_next);
  }
  
  // 2. Bottom hemisphere (from equator to south pole)
  const uint16_t bottom_hemi_start = static_cast<uint16_t>(vertices.size());
  for (int stack = 0; stack <= hemisphere_stacks; ++stack) {
    // phi goes from 0 (equator) to PI/2 (south pole)
    float phi = (static_cast<float>(stack) / hemisphere_stacks) * (DirectX::XM_PI * 0.5f);
    float y = -std::cos(phi) * radius - half_cyl_height;  // Negative y, offset down
    float ring_radius = std::sin(phi) * radius;
    
    for (int seg = 0; seg < segments; ++seg) {
      float theta = (static_cast<float>(seg) / segments) * DirectX::XM_2PI;
      float x = std::cos(theta) * ring_radius;
      float z = std::sin(theta) * ring_radius;
      
      float u = static_cast<float>(seg) / segments;
      // V maps from 0.25 (equator) down to 0.0 (south pole)
      float v = 0.25f - (static_cast<float>(stack) / hemisphere_stacks) * 0.25f;
      
      vertices.push_back({{x, y, z}, {u, v}});
    }
  }
  
  // Bottom hemisphere indices
  for (int stack = 0; stack < hemisphere_stacks; ++stack) {
    for (int seg = 0; seg < segments; ++seg) {
      int current = bottom_hemi_start + stack * segments + seg;
      int next_seg = (seg + 1) % segments;
      int next_stack = current + segments;
      int next_both = bottom_hemi_start + (stack + 1) * segments + next_seg;
      
      indices.push_back(static_cast<uint16_t>(current));
      indices.push_back(static_cast<uint16_t>(current + ((next_seg == 0) ? (1 - segments) : 1)));
      indices.push_back(static_cast<uint16_t>(next_stack));
      
      indices.push_back(static_cast<uint16_t>(current + ((next_seg == 0) ? (1 - segments) : 1)));
      indices.push_back(static_cast<uint16_t>(next_both));
      indices.push_back(static_cast<uint16_t>(next_stack));
    }
  }
  
  // 3. Top hemisphere (from equator to north pole)
  const uint16_t top_hemi_start = static_cast<uint16_t>(vertices.size());
  for (int stack = 0; stack <= hemisphere_stacks; ++stack) {
    // phi goes from 0 (equator) to PI/2 (north pole)
    float phi = (static_cast<float>(stack) / hemisphere_stacks) * (DirectX::XM_PI * 0.5f);
    float y = std::cos(phi) * radius + half_cyl_height;  // Positive y, offset up
    float ring_radius = std::sin(phi) * radius;
    
    for (int seg = 0; seg < segments; ++seg) {
      float theta = (static_cast<float>(seg) / segments) * DirectX::XM_2PI;
      float x = std::cos(theta) * ring_radius;
      float z = std::sin(theta) * ring_radius;
      
      float u = static_cast<float>(seg) / segments;
      // V maps from 0.75 (equator) up to 1.0 (north pole)
      float v = 0.75f + (static_cast<float>(stack) / hemisphere_stacks) * 0.25f;
      
      vertices.push_back({{x, y, z}, {u, v}});
    }
  }
  
  // Top hemisphere indices
  for (int stack = 0; stack < hemisphere_stacks; ++stack) {
    for (int seg = 0; seg < segments; ++seg) {
      int current = top_hemi_start + stack * segments + seg;
      int next_seg = (seg + 1) % segments;
      int next_stack = current + segments;
      int next_both = top_hemi_start + (stack + 1) * segments + next_seg;
      
      indices.push_back(static_cast<uint16_t>(current));
      indices.push_back(static_cast<uint16_t>(current + ((next_seg == 0) ? (1 - segments) : 1)));
      indices.push_back(static_cast<uint16_t>(next_stack));
      
      indices.push_back(static_cast<uint16_t>(current + ((next_seg == 0) ? (1 - segments) : 1)));
      indices.push_back(static_cast<uint16_t>(next_both));
      indices.push_back(static_cast<uint16_t>(next_stack));
    }
  }
  
  // Create vertex and index buffers
  auto vb = Buffer::CreateAndUploadToDefaultHeapForInit(
    device,
    upload_context,
    vertices.data(),
    vertices.size() * sizeof(VertexPositionTexture2D),
    Buffer::Type::Vertex,
    "Primitive_Capsule_VB"
  );
  
  auto ib = Buffer::CreateAndUploadToDefaultHeapForInit(
    device,
    upload_context,
    indices.data(),
    indices.size() * sizeof(uint16_t),
    Buffer::Type::Index,
    "Primitive_Capsule_IB"
  );
  
  // Create and initialize mesh
  auto mesh = std::make_shared<Mesh>();
  mesh->Initialize(
    vb,
    ib,
    sizeof(VertexPositionTexture2D),
    static_cast<uint32_t>(indices.size()),
    DXGI_FORMAT_R16_UINT,
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
  );
  mesh->SetDebugName("Primitive_Capsule");
  
  return mesh;
}
