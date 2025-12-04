#include "graphic.h"

#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgiformat.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>

#include "WICTextureLoader12.h"
#include "d3dx12.h"
#include "types.h"

using namespace DirectX;

struct Vertex {
  XMFLOAT3 pos;
  XMFLOAT2 uv;
};

struct TexRGBA {
  unsigned char R, G, B, A;
};

bool Graphic::Initalize(HWND hwnd, UINT frame_buffer_width, UINT frame_buffer_height) {
  frame_buffer_width_ = frame_buffer_width;
  frame_buffer_height_ = frame_buffer_height;

  std::wstring init_error_caption = L"Graphic Initialization Error";

  if (!CreateFactory()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create factory", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  if (!CreateDevice()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create device", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  if (!descriptor_heap_manager_.Initalize(device_.Get())) {
    MessageBoxW(nullptr, L"Graphic: Failed to initialize descriptor heap manager", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  if (!CreateCommandQueue()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create command queue", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  if (!CreateCommandAllocator()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create command allocator", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  if (!CreateCommandList()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create command list", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  if (!swap_chain_manager_.Initialize(device_.Get(),
        dxgi_factory_.Get(),
        command_queue_.Get(),
        hwnd,
        frame_buffer_width,
        frame_buffer_height,
        descriptor_heap_manager_)) {
    return false;
  }
  if (!depth_buffer_.Initialize(device_.Get(), frame_buffer_width, frame_buffer_height, descriptor_heap_manager_)) {
    return false;
  }
  if (!fence_manager_.Initialize(device_.Get())) {
    return false;
  }

  // NOLINTBEGIN
  HRESULT hr;

  // Draw Triangle
  Vertex vertices[] = {
    {{-0.4f, -0.7f, 0.0f}, {0.0f, 1.0f}},  // 左下
    {{-0.4f, 0.7f, 0.0f}, {0.0f, 0.0f}},   // 左上
    {{0.4f, -0.7f, 0.0f}, {1.0f, 1.0f}},   // 右下
    {{0.4f, 0.7f, 0.0f}, {1.0f, 0.0f}},    // 右上
  };

  // Create vertex buffer
  D3D12_HEAP_PROPERTIES heapprop = {};
  heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

  D3D12_RESOURCE_DESC resdesc = {};
  resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resdesc.Width = sizeof(vertices);
  resdesc.Height = 1;
  resdesc.DepthOrArraySize = 1;
  resdesc.MipLevels = 1;
  resdesc.Format = DXGI_FORMAT_UNKNOWN;
  resdesc.SampleDesc.Count = 1;
  resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ID3D12Resource* vertBuff = nullptr;
  hr = device_->CreateCommittedResource(
    &heapprop, D3D12_HEAP_FLAG_NONE, &resdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertBuff));

  if (FAILED(hr)) {
    std::cerr << "Failed to create vertex buffer." << std::endl;
    return false;
  }

  Vertex* vertMap = nullptr;
  hr = vertBuff->Map(0, nullptr, (void**)&vertMap);
  std::copy(std::begin(vertices), std::end(vertices), vertMap);
  vertBuff->Unmap(0, nullptr);

  vertex_buffer_view_.BufferLocation = vertBuff->GetGPUVirtualAddress();  // バッファの仮想アドレス
  vertex_buffer_view_.SizeInBytes = sizeof(vertices);                     // 全バイト数
  vertex_buffer_view_.StrideInBytes = sizeof(vertices[0]);                // 1頂点あたりのバイト数

  unsigned short indices[] = {0, 1, 2, 2, 1, 3};

  ID3D12Resource* idxBuff = nullptr;

  resdesc.Width = sizeof(indices);
  hr = device_->CreateCommittedResource(
    &heapprop, D3D12_HEAP_FLAG_NONE, &resdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&idxBuff));

  unsigned short* mappedIdx = nullptr;
  idxBuff->Map(0, nullptr, reinterpret_cast<void**>(&mappedIdx));
  std::copy(std::begin(indices), std::end(indices), mappedIdx);
  idxBuff->Unmap(0, nullptr);

  index_buffer_view_.BufferLocation = idxBuff->GetGPUVirtualAddress();
  index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;
  index_buffer_view_.SizeInBytes = sizeof(indices);

  // Read Shader
  ID3DBlob* _vsBlob = nullptr;
  ID3DBlob* _psBlob = nullptr;

  if (FAILED(D3DReadFileToBlob(L"Content/shaders/basic.vs.cso", &_vsBlob))) {
    throw std::runtime_error("Failed to read vertex shader");
  }

  if (FAILED(D3DReadFileToBlob(L"Content/shaders/basic.ps.cso", &_psBlob))) {
    throw std::runtime_error("Failed to read pixel shader");
  }

  D3D12_SHADER_BYTECODE vsBytecode{};
  vsBytecode.pShaderBytecode = _vsBlob->GetBufferPointer();
  vsBytecode.BytecodeLength = _vsBlob->GetBufferSize();

  D3D12_SHADER_BYTECODE psBytecode{};
  psBytecode.pShaderBytecode = _psBlob->GetBufferPointer();
  psBytecode.BytecodeLength = _psBlob->GetBufferSize();

  // Input layout
  D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

  // Graphics pipeline
  D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
  gpipeline.pRootSignature = nullptr;
  gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
  gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
  gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
  gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

  // デフォルトのサンプルマスクを表す定数(0xffffffff)
  gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  // まだアンチエイリアスは使わないためfalse
  gpipeline.RasterizerState.MultisampleEnable = FALSE;
  gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;   // カリングしない
  gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;  // 中身を塗りつぶす
  gpipeline.RasterizerState.DepthClipEnable = TRUE;            // 深度方向のクリッピングは有効に

  // Alpha Blend
  gpipeline.BlendState.AlphaToCoverageEnable = FALSE;
  gpipeline.BlendState.IndependentBlendEnable = FALSE;

  D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

  // ひとまず加算や乗算やαブレンディングは使用しない
  renderTargetBlendDesc.BlendEnable = FALSE;
  renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  // ひとまず論理演算は使用しない
  renderTargetBlendDesc.LogicOpEnable = FALSE;

  gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

  // Rasterizer State
  gpipeline.RasterizerState.MultisampleEnable = FALSE;         // まだアンチェリは使わない
  gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;   // カリングしない
  gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;  // 中身を塗りつぶす
  gpipeline.RasterizerState.DepthClipEnable = TRUE;            // 深度方向のクリッピングは有効に

  // 残り
  gpipeline.RasterizerState.FrontCounterClockwise = FALSE;
  gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  gpipeline.RasterizerState.AntialiasedLineEnable = FALSE;
  gpipeline.RasterizerState.ForcedSampleCount = 0;
  gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  gpipeline.InputLayout.pInputElementDescs = inputLayout;     // レイアウト先頭アドレス
  gpipeline.InputLayout.NumElements = _countof(inputLayout);  // レイアウト配列数

  // Depth stencil
  gpipeline.DepthStencilState.DepthEnable = FALSE;
  gpipeline.DepthStencilState.StencilEnable = FALSE;

  // Input layout
  gpipeline.InputLayout.pInputElementDescs = inputLayout;     // レイアウト先頭アドレス
  gpipeline.InputLayout.NumElements = _countof(inputLayout);  // レイアウト配列数

  // Vertex related setting
  gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;   // ストリップ時のカットなし
  gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;  // 三角形で構成

  // Render target
  gpipeline.NumRenderTargets = 1;                        // 今は１つのみ
  gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;  // 0～1に正規化されたRGBA

  // Sample
  gpipeline.SampleDesc.Count = 1;    // サンプリングは1ピクセルにつき１
  gpipeline.SampleDesc.Quality = 0;  // クオリティは最低

  // Root signature

  D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
  rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  // Root parameter
  D3D12_DESCRIPTOR_RANGE descTblRange = {};
  descTblRange.NumDescriptors = 1;                           // テクスチャひとつ
  descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;  // 種別はテクスチャ
  descTblRange.BaseShaderRegister = 0;                       // 0番スロットから
  descTblRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  D3D12_ROOT_PARAMETER rootparam = {};
  rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;  // デスクリプタレンジのアドレス
  rootparam.DescriptorTable.NumDescriptorRanges = 1;            // デスクリプタレンジ数
  rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;   // ピクセルシェーダから見える

  rootSignatureDesc.pParameters = &rootparam;  // ルートパラメータの先頭アドレス
  rootSignatureDesc.NumParameters = 1;         // ルートパラメータ数

  // Sampler
  D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
  samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                 // 横繰り返し
  samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                 // 縦繰り返し
  samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                 // 奥行繰り返し
  samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;  // ボーダーの時は黒
  samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;                    // 補間しない(ニアレストネイバー)
  samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;                                 // ミップマップ最大値
  samplerDesc.MinLOD = 0.0f;                                              // ミップマップ最小値
  samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;               // オーバーサンプリングの際リサンプリングしない？
  samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;           // ピクセルシェーダからのみ可視

  rootSignatureDesc.pStaticSamplers = &samplerDesc;
  rootSignatureDesc.NumStaticSamplers = 1;

  ID3DBlob* errorBlob = nullptr;
  ID3DBlob* rootSigBlob = nullptr;
  hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
  hr = device_->CreateRootSignature(
    0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(root_signature_.GetAddressOf()));
  rootSigBlob->Release();

  gpipeline.pRootSignature = root_signature_.Get();
  hr = device_->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(pipeline_state_.GetAddressOf()));

  viewport_.Width = static_cast<FLOAT>(frame_buffer_width_);    // 出力先の幅(ピクセル数)
  viewport_.Height = static_cast<FLOAT>(frame_buffer_height_);  // 出力先の高さ(ピクセル数)
  viewport_.TopLeftX = 0;                                       // 出力先の左上座標X
  viewport_.TopLeftY = 0;                                       // 出力先の左上座標Y
  viewport_.MaxDepth = 1.0f;                                    // 深度最大値
  viewport_.MinDepth = 0.0f;                                    // 深度最小値

  scissor_rect_.top = 0;                                            // 切り抜き上座標
  scissor_rect_.left = 0;                                           // 切り抜き左座標
  scissor_rect_.right = scissor_rect_.left + frame_buffer_width_;   // 切り抜き右座標
  scissor_rect_.bottom = scissor_rect_.top + frame_buffer_height_;  // 切り抜き下座標

#pragma region debug_load_texture
  // Load Texture
  std::unique_ptr<uint8_t[]> decodedData;
  D3D12_SUBRESOURCE_DATA subresource;
  hr = LoadWICTextureFromFile(
    device_.Get(), L"Content/textures/metal_plate_diff_1k.png", texture_buffer_.GetAddressOf(), decodedData, subresource);

  if (FAILED(hr) || texture_buffer_ == nullptr) {
    std::cerr << "Failed to load texture." << std::endl;
    return false;
  }

  D3D12_RESOURCE_DESC textureDesc = texture_buffer_->GetDesc();

  // Configure footprint
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
  UINT numRows = 0;
  UINT64 rowSizeInBytes = 0;
  UINT64 totalBytes = 0;

  device_->GetCopyableFootprints(&textureDesc,
    0,                // FirstSubresource
    1,                // NumSubresources
    0,                // BaseOffset
    &footprint,       // 輸出的佈局結構
    &numRows,         // 輸出的行數 (Height)
    &rowSizeInBytes,  // 輸出的每行實際數據大小 (不含對齊 Padding)
    &totalBytes       // 輸出的總 Buffer 大小
  );

  // Create upload buffer
  CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
  auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);

  ComPtr<ID3D12Resource> textureUpload;
  hr = device_->CreateCommittedResource(
    &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUpload));

  if (FAILED(hr)) {
    std::cerr << "Failed to create texture upload buffer." << '\n';
    return false;
  }

  // Copy CPU data to upload buffer
  UINT8* pMappedData = nullptr;
  hr = textureUpload->Map(0, nullptr, reinterpret_cast<void**>(&pMappedData));
  if (FAILED(hr)) {
    std::cerr << "Failed to map texture upload buffer." << '\n';
    return false;
  }

  const UINT8* pSrcData = reinterpret_cast<const UINT8*>(subresource.pData);
  for (UINT y = 0; y < numRows; ++y) {
    memcpy(pMappedData + footprint.Offset + (static_cast<size_t>(y * footprint.Footprint.RowPitch)),
      pSrcData + (y * subresource.RowPitch),
      static_cast<size_t>(rowSizeInBytes));
  }
  textureUpload->Unmap(0, nullptr);

  command_allocator_->Reset();
  command_list_->Reset(command_allocator_.Get(), nullptr);

  D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
  srcLocation.pResource = textureUpload.Get();
  srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  srcLocation.PlacedFootprint = footprint;

  D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
  dstLocation.pResource = texture_buffer_.Get();
  dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  dstLocation.SubresourceIndex = 0;

  command_list_->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

  auto texBarrier =
    CD3DX12_RESOURCE_BARRIER::Transition(texture_buffer_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  command_list_->ResourceBarrier(1, &texBarrier);
  command_list_->Close();

  ID3D12CommandList* uploadCmds[] = {command_list_.Get()};
  command_queue_->ExecuteCommandLists(1, uploadCmds);

  fence_manager_.WaitForGpu(command_queue_.Get());

  // texture shader resource heap
  D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
  descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;                              // シェーダから見えるように
  descHeapDesc.NodeMask = 0;                                                                   // マスクは0
  descHeapDesc.NumDescriptors = 1;                                                             // ビューは今のところ１つだけ
  descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;                                  // シェーダリソースビュー(および定数、UAVも)
  hr = device_->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texture_descriptor_heap_));  // 生成

  if (FAILED(hr) || texture_descriptor_heap_ == nullptr) {
    std::cerr << "Failed to create texture descriptor heap." << std::endl;
    return false;
  }

  // 通常テクスチャビュー作成
  D3D12_RESOURCE_DESC texDesc = texture_buffer_->GetDesc();
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = texDesc.Format;                                             // DXGI_FORMAT_R8G8B8A8_UNORM;//RGBA(0.0f～1.0fに正規化)
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;  // 後述
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                       // 2Dテクスチャ
  srvDesc.Texture2D.MipLevels = texDesc.MipLevels;                             // ミップマップは使用しないので1

  device_->CreateShaderResourceView(texture_buffer_.Get(),          // ビューと関連付けるバッファ
    &srvDesc,                                                       // 先ほど設定したテクスチャ設定情報
    texture_descriptor_heap_->GetCPUDescriptorHandleForHeapStart()  // ヒープのどこに割り当てるか
  );
#pragma endregion debug_load_texture
  // NOLINTEND

  return true;
}

void Graphic::BeginRender() {
  command_allocator_->Reset();
  command_list_->Reset(command_allocator_.Get(), nullptr);

  descriptor_heap_manager_.BeginFrame();
  descriptor_heap_manager_.SetDescriptorHeaps(command_list_.Get());

  swap_chain_manager_.TransitionToRenderTarget(command_list_.Get());

  D3D12_CPU_DESCRIPTOR_HANDLE rtv = swap_chain_manager_.GetCurrentRTV();
  D3D12_CPU_DESCRIPTOR_HANDLE dsv = depth_buffer_.GetDSV();

  command_list_->RSSetViewports(1, &viewport_);
  command_list_->RSSetScissorRects(1, &scissor_rect_);

  std::array<float, 4> clearColor = {1.0f, 1.0f, 0.0f, 1.0f};
  command_list_->ClearRenderTargetView(rtv, clearColor.data(), 0, nullptr);
  depth_buffer_.Clear(command_list_.Get(), 1.0, 0);

  command_list_->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

  // Drawing Logic
  command_list_->SetPipelineState(pipeline_state_.Get());
  command_list_->RSSetViewports(1, &viewport_);
  command_list_->RSSetScissorRects(1, &scissor_rect_);
  command_list_->SetGraphicsRootSignature(root_signature_.Get());

  command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  command_list_->IASetVertexBuffers(0, 1, &vertex_buffer_view_);
  command_list_->IASetIndexBuffer(&index_buffer_view_);

  command_list_->SetGraphicsRootSignature(root_signature_.Get());

  std::array<ID3D12DescriptorHeap*, 1> heaps = {texture_descriptor_heap_.Get()};
  command_list_->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
  command_list_->SetGraphicsRootDescriptorTable(0, texture_descriptor_heap_->GetGPUDescriptorHandleForHeapStart());

  constexpr int index_count = 6;
  command_list_->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
}

void Graphic::EndRender() {
  swap_chain_manager_.TransitionToPresent(command_list_.Get());

  command_list_->Close();

  std::array<ID3D12CommandList*, 1> cmdlists = {command_list_.Get()};
  command_queue_->ExecuteCommandLists(static_cast<UINT>(cmdlists.size()), cmdlists.data());

  fence_manager_.WaitForGpu(command_queue_.Get());

  swap_chain_manager_.Present(1, 0);
}

bool Graphic::EnableDebugLayer() {
  ID3D12Debug* debugLayer = nullptr;

  if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))) {
    return false;
  }

  debugLayer->EnableDebugLayer();
  debugLayer->Release();

  return true;
}

bool Graphic::CreateFactory() {
#if defined(DEBUG) || defined(_DEBUG)
  EnableDebugLayer();
  UINT dxgi_factory_flag = DXGI_CREATE_FACTORY_DEBUG;
#else
  UINT dxgi_factory_flag = 0;
#endif

  auto hr = CreateDXGIFactory2(dxgi_factory_flag, IID_PPV_ARGS(&dxgi_factory_));
  if (FAILED(hr)) {
    std::cerr << "Failed to create dxgi factory." << '\n';
    return false;
  }
  return true;
}

bool Graphic::CreateDevice() {
  std::vector<ComPtr<IDXGIAdapter>> adapters;
  ComPtr<IDXGIAdapter> tmpAdapter = nullptr;

  for (int i = 0; dxgi_factory_->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
    adapters.push_back(tmpAdapter);
  }

  for (const auto& adapter : adapters) {
    DXGI_ADAPTER_DESC adapter_desc = {};
    adapter->GetDesc(&adapter_desc);
    if (std::wstring desc_str = adapter_desc.Description;  // NOLINT (cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      desc_str.find(L"NVIDIA") != std::string::npos) {
      tmpAdapter = adapter;
      break;
    }
  }

  std::array<D3D_FEATURE_LEVEL, 5> levels = {
    D3D_FEATURE_LEVEL_12_2,
    D3D_FEATURE_LEVEL_12_1,
    D3D_FEATURE_LEVEL_12_0,
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
  };

  for (auto level : levels) {
    auto hr = D3D12CreateDevice(tmpAdapter.Get(), level, IID_PPV_ARGS(&device_));
    if (SUCCEEDED(hr) && device_ != nullptr) {
      return true;
    }
  }

  std::cerr << "Failed to create device." << '\n';
  return false;
}

bool Graphic::CreateCommandQueue() {
  D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
  command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  command_queue_desc.NodeMask = 0;
  command_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

  auto hr = device_->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&command_queue_));
  if (FAILED(hr) || command_queue_ == nullptr) {
    std::cerr << "Failed to create command queue." << '\n';
    return false;
  }
  return true;
}

bool Graphic::CreateCommandList() {
  auto hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator_.Get(), nullptr, IID_PPV_ARGS(&command_list_));

  if (FAILED(hr) || command_list_ == nullptr) {
    std::cerr << "Failed to create command list." << '\n';
    return false;
  }

  command_list_->Close();
  return true;
}

bool Graphic::CreateCommandAllocator() {
  auto hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_));

  if (FAILED(hr) || command_allocator_ == nullptr) {
    std::cerr << "Failed to create command allocator." << '\n';
    return false;
  }

  return true;
}
