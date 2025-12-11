#include "framework_default_assets.h"

#include <cstdint>
#include <iostream>
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


void FrameworkDefaultAssets::Initialize(Graphic& graphic) {
  graphic_ = &graphic;

  // Create a simple rect mesh using the primitive geometry helper.
  try {
    rect2d_mesh_ = graphic.GetPrimitiveGeometry2D().CreateRect();
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
  // Release material instances first
  sprite2d_default_.reset();
  debug_line_overlay_default_.reset();
  debug_line_depth_default_.reset();

  // Release textures from managers (Graphic still owns managers at this point)
  if (graphic_) {
    auto& tex_mgr = graphic_->GetTextureManager();
    if (tex_mgr.IsValid(white_texture_)) tex_mgr.ReleaseTexture(white_texture_);
    if (tex_mgr.IsValid(black_texture_)) tex_mgr.ReleaseTexture(black_texture_);
    if (tex_mgr.IsValid(flat_normal_texture_)) tex_mgr.ReleaseTexture(flat_normal_texture_);
    if (tex_mgr.IsValid(error_texture_)) tex_mgr.ReleaseTexture(error_texture_);
  }

  // Reset all handles and pointers
  white_texture_ = INVALID_TEXTURE_HANDLE;
  black_texture_ = INVALID_TEXTURE_HANDLE;
  flat_normal_texture_ = INVALID_TEXTURE_HANDLE;
  error_texture_ = INVALID_TEXTURE_HANDLE;

  graphic_ = nullptr;
  rect2d_mesh_.reset();
  sprite2d_template_ = nullptr;
  debug_line_template_overlay_ = nullptr;
  debug_line_template_depth_ = nullptr;
  sprite_material_ = nullptr;
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
  return sprite_material_;
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
      std::cerr << "[FrameworkDefaultAssets] Failed to load BasicVS shader" << '\n';
      return;
    }
  }

  if (!shader_mgr.HasShader("BasicPS")) {
    if (!shader_mgr.LoadShader(L"Content/shaders/basic.ps.cso", ShaderType::Pixel, "BasicPS")) {
      std::cerr << "[FrameworkDefaultAssets] Failed to load BasicPS shader" << '\n';
      return;
    }
  }

  if (!shader_mgr.HasShader("DebugLineVS")) {
    if (!shader_mgr.LoadShader(L"Content/shaders/debug_line.vs.cso", ShaderType::Vertex, "DebugLineVS")) {
      std::cerr << "[FrameworkDefaultAssets] Failed to load DebugLineVS shader" << '\n';
      return;
    }
  }

  if (!shader_mgr.HasShader("DebugLinePS")) {
    if (!shader_mgr.LoadShader(L"Content/shaders/debug_line.ps.cso", ShaderType::Pixel, "DebugLinePS")) {
      std::cerr << "[FrameworkDefaultAssets] Failed to load DebugLinePS shader" << '\n';
      return;
    }
  }

  // Create Sprite2D material
  CreateSprite2DMaterial(gfx);

  // Create DebugLine materials (overlay and depth-tested)
  CreateDebugLineMaterials(gfx);
}

void FrameworkDefaultAssets::CreateSprite2DMaterial(Graphic& gfx) {
  auto& shader_mgr = gfx.GetShaderManager();
  auto& material_mgr = gfx.GetMaterialManager();

  // Create root signature for Sprite2D
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
    std::cerr << "[FrameworkDefaultAssets] Failed to create Sprite2D root signature" << '\n';
    return;
  }

  // Create pipeline state for Sprite2D
  ComPtr<ID3D12PipelineState> sprite_pso;
  PipelineStateBuilder pso_builder;
  const ShaderBlob* vs = shader_mgr.GetShader("BasicVS");
  const ShaderBlob* ps = shader_mgr.GetShader("BasicPS");

  auto input_layout = GetInputLayout_VertexPositionTexture2D();

  pso_builder.SetVertexShader(vs)
    .SetPixelShader(ps)
    .SetInputLayout(input_layout.data(), static_cast<UINT>(input_layout.size()))
    .SetRootSignature(sprite_root_signature.Get())
    .SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
    .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)  // Standard back buffer format
    .SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT)
    .UseForwardTransparentDefaults();  // This should enable alpha blending

  if (!pso_builder.Build(gfx.GetDevice(), sprite_pso)) {
    std::cerr << "[FrameworkDefaultAssets] Failed to create Sprite2D PSO" << '\n';
    return;
  }

  // Create material template
  std::vector<TextureSlotDefinition> sprite_texture_slots = {
    {"BaseColor", 4, D3D12_SHADER_VISIBILITY_PIXEL}  // t0, parameter index 4 (descriptor table)
  };

  sprite2d_template_ = material_mgr.CreateTemplate("DefaultSprite2D", sprite_pso.Get(), sprite_root_signature.Get(), sprite_texture_slots);

  if (sprite2d_template_) {
    // Create default instance and bind white texture
    sprite2d_default_ = std::make_unique<MaterialInstance>();
    if (sprite2d_default_->Initialize(sprite2d_template_)) {
      sprite2d_default_->SetTexture("BaseColor", white_texture_);
      sprite_material_ = sprite2d_default_.get();
      std::cout << "[FrameworkDefaultAssets] Created Sprite2D default material" << '\n';
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
    std::cerr << "[FrameworkDefaultAssets] Failed to create DebugLine root signature" << '\n';
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
      std::cerr << "[FrameworkDefaultAssets] Failed to create DebugLine overlay PSO" << '\n';
      return;
    }

    debug_line_template_overlay_ =
      material_mgr.CreateTemplate("DefaultDebugLineOverlay", overlay_pso.Get(), debug_root_signature.Get());

    if (debug_line_template_overlay_) {
      debug_line_overlay_default_ = std::make_unique<MaterialInstance>();
      if (debug_line_overlay_default_->Initialize(debug_line_template_overlay_)) {
        debug_line_material_overlay_ = debug_line_overlay_default_.get();
        std::cout << "[FrameworkDefaultAssets] Created DebugLine overlay material" << '\n';
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
      std::cerr << "[FrameworkDefaultAssets] Failed to create DebugLine depth PSO" << '\n';
      return;
    }

    debug_line_template_depth_ = material_mgr.CreateTemplate("DefaultDebugLineDepth", depth_pso.Get(), debug_root_signature.Get());

    if (debug_line_template_depth_) {
      debug_line_depth_default_ = std::make_unique<MaterialInstance>();
      if (debug_line_depth_default_->Initialize(debug_line_template_depth_)) {
        debug_line_material_depth_ = debug_line_depth_default_.get();
        std::cout << "[FrameworkDefaultAssets] Created DebugLine depth material" << '\n';
      }
    }
  }
}
