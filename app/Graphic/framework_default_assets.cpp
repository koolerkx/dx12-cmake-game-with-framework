#include "framework_default_assets.h"

#include <cstdint>
#include <vector>

#include "graphic.h"
#include "material_instance.h"
#include "material_manager.h"
#include "material_template.h"
#include "pipeline_state_builder.h"
#include "primitive_geometry_2d.h"
#include "root_signature_builder.h"
#include "shader_manager.h"
#include "texture_manager.h"
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
  sprite_world_opaque_template_ = nullptr;
  sprite_world_transparent_template_ = nullptr;
  sprite_ui_template_ = nullptr;
  debug_line_template_overlay_ = nullptr;
  debug_line_template_depth_ = nullptr;
  sprite_world_opaque_material_ = nullptr;
  sprite_world_transparent_material_ = nullptr;
  sprite_ui_material_ = nullptr;
  debug_line_material_overlay_ = nullptr;
  debug_line_material_depth_ = nullptr;
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
  ComPtr<ID3D12RootSignature> debug_root_signature;
  RootSignatureBuilder rs_builder;
  rs_builder
    .AddRootConstant(16, 0, D3D12_SHADER_VISIBILITY_VERTEX)  // b0 - World matrix (row-major float4x4)
    .AddRootCBV(1, D3D12_SHADER_VISIBILITY_VERTEX)           // b1 - FrameCB (view/proj)
    .AllowInputLayout();

  if (!rs_builder.Build(gfx.GetDevice(), debug_root_signature)) {
     Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FrameworkDefaultAssets] Failed to create DebugLine root signature");
    return;
  }

  const ShaderBlob* vs = shader_mgr.GetShader("DebugLineVS");
  const ShaderBlob* ps = shader_mgr.GetShader("DebugLinePS");
  auto input_layout = GetInputLayout_DebugVertex();

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
}
