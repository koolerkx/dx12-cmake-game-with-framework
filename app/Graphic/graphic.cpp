#include "graphic.h"

#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgiformat.h>
#include <winerror.h>
#include <winnt.h>

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
  if (!CreateCommandQueue()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create command queue", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  if (!CreateSwapChain(hwnd, frame_buffer_width_, frame_buffer_height_)) {
    MessageBoxW(nullptr, L"Graphic: Failed to create swap chain", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  if (!CreateDescriptorHeapForFrameBuffer()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create descriptor heap", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  if (!CreateRTVForFameBuffer()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create RTV", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  if (!CreateDSVForFrameBuffer()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create DSV", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
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

  if (!CreateSynchronizationWithGPUObject()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create fence", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

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

  command_list_->IASetVertexBuffers(0, 1, &vertex_buffer_view_);

  unsigned short indices[] = {0, 1, 2, 2, 1, 3};

  ID3D12Resource* idxBuff = nullptr;

  resdesc.Width = sizeof(indices);
  hr = device_->CreateCommittedResource(
    &heapprop, D3D12_HEAP_FLAG_NONE, &resdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&idxBuff));

  unsigned short* mappedIdx = nullptr;
  idxBuff->Map(0, nullptr, (void**)&mappedIdx);
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
  gpipeline.RasterizerState.MultisampleEnable = false;
  gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;   // カリングしない
  gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;  // 中身を塗りつぶす
  gpipeline.RasterizerState.DepthClipEnable = true;            // 深度方向のクリッピングは有効に

  // Alpha Blend
  gpipeline.BlendState.AlphaToCoverageEnable = false;
  gpipeline.BlendState.IndependentBlendEnable = false;

  D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

  // ひとまず加算や乗算やαブレンディングは使用しない
  renderTargetBlendDesc.BlendEnable = false;
  renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  // ひとまず論理演算は使用しない
  renderTargetBlendDesc.LogicOpEnable = false;

  gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

  // Rasterizer State
  gpipeline.RasterizerState.MultisampleEnable = false;         // まだアンチェリは使わない
  gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;   // カリングしない
  gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;  // 中身を塗りつぶす
  gpipeline.RasterizerState.DepthClipEnable = true;            // 深度方向のクリッピングは有効に

  // 残り
  gpipeline.RasterizerState.FrontCounterClockwise = false;
  gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  gpipeline.RasterizerState.AntialiasedLineEnable = false;
  gpipeline.RasterizerState.ForcedSampleCount = 0;
  gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  gpipeline.InputLayout.pInputElementDescs = inputLayout;     // レイアウト先頭アドレス
  gpipeline.InputLayout.NumElements = _countof(inputLayout);  // レイアウト配列数

  // Depth stencil
  gpipeline.DepthStencilState.DepthEnable = false;
  gpipeline.DepthStencilState.StencilEnable = false;

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

  // Texture
  std::vector<TexRGBA> texturedata(256 * 256);
  for (auto& rgba : texturedata) {
    rgba.R = rand() % 256;
    rgba.G = rand() % 256;
    rgba.B = rand() % 256;
    rgba.A = 255;  // αは 1.0 とする
  }

  D3D12_HEAP_PROPERTIES texHeapProp = {};
  texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;                         // 特殊な設定なのでdefaultでもuploadでもなく
  texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;  // ライトバックで
  texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;           // 転送がL0つまりCPU側から直で
  texHeapProp.CreationNodeMask = 0;                                  // 単一アダプタのため0
  texHeapProp.VisibleNodeMask = 0;                                   // 単一アダプタのため0

  D3D12_RESOURCE_DESC resDesc = {};
  resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;             // DXGI_FORMAT_R8G8B8A8_UNORM;//RGBAフォーマット
  resDesc.Width = static_cast<UINT>(256);                  // 幅
  resDesc.Height = static_cast<UINT>(256);                 // 高さ
  resDesc.DepthOrArraySize = static_cast<uint16_t>(1);     // 2Dで配列でもないので１
  resDesc.SampleDesc.Count = 1;                            // 通常テクスチャなのでアンチェリしない
  resDesc.SampleDesc.Quality = 0;                          //
  resDesc.MipLevels = static_cast<uint16_t>(1);            // ミップマップしないのでミップ数は１つ
  resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // 2Dテクスチャ用
  resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;           // レイアウトについては決定しない
  resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;                // とくにフラグなし

  ID3D12Resource* texbuff = nullptr;
  hr = device_->CreateCommittedResource(&texHeapProp,
    D3D12_HEAP_FLAG_NONE,  // 特に指定なし
    &resDesc,
    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,  // テクスチャ用(ピクセルシェーダから見る用)
    nullptr,
    IID_PPV_ARGS(&texbuff));

  hr = texbuff->WriteToSubresource(0,
    nullptr,                                                 // 全領域へコピー
    texturedata.data(),                                      // 元データアドレス
    sizeof(TexRGBA) * 256,                                   // 1 ラインサイズ
    static_cast<UINT>(sizeof(TexRGBA) * texturedata.size())  // 全サイズ
  );

  // texture shader resource heap
  D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
  descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;                              // シェーダから見えるように
  descHeapDesc.NodeMask = 0;                                                                   // マスクは0
  descHeapDesc.NumDescriptors = 1;                                                             // ビューは今のところ１つだけ
  descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;                                  // シェーダリソースビュー(および定数、UAVも)
  hr = device_->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texture_descriptor_heap_));  // 生成

  // 通常テクスチャビュー作成
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;                                 // DXGI_FORMAT_R8G8B8A8_UNORM;//RGBA(0.0f～1.0fに正規化)
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;  // 後述
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                       // 2Dテクスチャ
  srvDesc.Texture2D.MipLevels = 1;                                             // ミップマップは使用しないので1

  device_->CreateShaderResourceView(texbuff,                        // ビューと関連付けるバッファ
    &srvDesc,                                                       // 先ほど設定したテクスチャ設定情報
    texture_descriptor_heap_->GetCPUDescriptorHandleForHeapStart()  // ヒープのどこに割り当てるか
  );

  return true;
}

void Graphic::BeginRender() {
  frame_index_ = swap_chain_->GetCurrentBackBufferIndex();
  command_allocator_->Reset();

  command_list_->Reset(command_allocator_.Get(), nullptr);

  D3D12_RESOURCE_BARRIER BarrierDesc = {};
  BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  BarrierDesc.Transition.pResource = _backBuffers[frame_index_].Get();
  BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  command_list_->ResourceBarrier(1, &BarrierDesc);

  auto rtvH = rtv_heaps_->GetCPUDescriptorHandleForHeapStart();
  rtvH.ptr += static_cast<ULONG_PTR>(frame_index_ * device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
  command_list_->OMSetRenderTargets(1, &rtvH, false, nullptr);

  float clearColor[] = {1.0f, 1.0f, 0.0f, 1.0f};
  command_list_->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

  command_list_->SetPipelineState(pipeline_state_.Get());
  command_list_->RSSetViewports(1, &viewport_);
  command_list_->RSSetScissorRects(1, &scissor_rect_);
  command_list_->SetGraphicsRootSignature(root_signature_.Get());

  command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  command_list_->IASetVertexBuffers(0, 1, &vertex_buffer_view_);
  command_list_->IASetIndexBuffer(&index_buffer_view_);

  command_list_->SetGraphicsRootSignature(root_signature_.Get());
  command_list_->SetDescriptorHeaps(1, texture_descriptor_heap_.GetAddressOf());
  command_list_->SetGraphicsRootDescriptorTable(0, texture_descriptor_heap_->GetGPUDescriptorHandleForHeapStart());

  command_list_->DrawIndexedInstanced(6, 1, 0, 0, 0);

  BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  command_list_->ResourceBarrier(1, &BarrierDesc);

  command_list_->Close();

  ID3D12CommandList* cmdlists[] = {command_list_.Get()};
  command_queue_->ExecuteCommandLists(1, cmdlists);
  command_queue_->Signal(fence_.Get(), ++fence_val_);

  if (fence_->GetCompletedValue() != fence_val_) {
    auto event = CreateEvent(nullptr, false, false, nullptr);
    fence_->SetEventOnCompletion(fence_val_, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
  }

  swap_chain_->Present(1, 0);
}

void Graphic::EndRender() {
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

  for (auto adapter : adapters) {
    DXGI_ADAPTER_DESC adapter_desc = {};
    adapter->GetDesc(&adapter_desc);
    if (std::wstring desc_str = adapter_desc.Description; desc_str.find(L"NVIDIA") != std::string::npos) {
      tmpAdapter = adapter;
      break;
    }
  }

  D3D_FEATURE_LEVEL levels[] = {
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

  std::cerr << "Failed to create device." << std::endl;
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
    std::cerr << "Failed to create command queue." << std::endl;
    return false;
  }
  return true;
}

bool Graphic::CreateCommandList() {
  auto hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator_.Get(), nullptr, IID_PPV_ARGS(&command_list_));

  if (FAILED(hr) || command_list_ == nullptr) {
    std::cerr << "Failed to create command list." << std::endl;
    return false;
  }

  command_list_->Close();
  return true;
}

bool Graphic::CreateCommandAllocator() {
  auto hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_));

  if (FAILED(hr) || command_allocator_ == nullptr) {
    std::cerr << "Failed to create command allocator." << std::endl;
    return false;
  }
  return true;
}

bool Graphic::CreateSwapChain(HWND hwnd, UINT frameBufferWidth, UINT frameBufferHeight) {
  DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
  swap_chain_desc.Width = frameBufferWidth;
  swap_chain_desc.Height = frameBufferHeight;
  swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swap_chain_desc.Stereo = false;
  swap_chain_desc.SampleDesc.Count = 1;
  swap_chain_desc.SampleDesc.Quality = 0;
  swap_chain_desc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
  swap_chain_desc.BufferCount = 2;
  swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
  swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  ComPtr<IDXGISwapChain1> swap_chain1;
  HRESULT hr =
    dxgi_factory_->CreateSwapChainForHwnd(command_queue_.Get(), hwnd, &swap_chain_desc, nullptr, nullptr, (swap_chain1.GetAddressOf()));

  if (FAILED(hr) || swap_chain1 == nullptr) {
    std::cerr << "Failed to create swap chain." << std::endl;
    return false;
  }

  swap_chain1.As(&swap_chain_);

  if (FAILED(hr) || swap_chain_ == nullptr) {
    std::cerr << "Failed to create swap chain." << std::endl;
    return false;
  }

  return true;
}

bool Graphic::CreateDescriptorHeapForFrameBuffer() {
  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.NumDescriptors = FRAME_BUFFER_COUNT;
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  heap_desc.NodeMask = 0;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  auto hr = device_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtv_heaps_));

  rtv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  if (FAILED(hr) || rtv_heaps_ == nullptr) {
    std::cerr << "Failed to create RTV descriptor heap." << std::endl;
    return false;
  }

  heap_desc.NumDescriptors = 1;
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  hr = device_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&dsv_heaps_));
  if (FAILED(hr) || dsv_heaps_ == nullptr) {
    std::cerr << "Failed to create DSV descriptor heap." << std::endl;
    return false;
  }

  dsv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

  return true;
}

bool Graphic::CreateRTVForFameBuffer() {
  if (swap_chain_ == nullptr) {
    std::cerr << "Failed to create RTV for frame buffer, swap_chain_ is nullptr." << std::endl;
    return false;
  }

  if (rtv_heaps_ == nullptr) {
    std::cerr << "Failed to create RTV for frame buffer, rtv_heaps_ is nullptr." << std::endl;
    return false;
  }

  DXGI_SWAP_CHAIN_DESC swcDesc = {};
  swap_chain_->GetDesc(&swcDesc);

  D3D12_CPU_DESCRIPTOR_HANDLE handle = rtv_heaps_->GetCPUDescriptorHandleForHeapStart();
  for (size_t i = 0; i < swcDesc.BufferCount; ++i) {
    swap_chain_->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(_backBuffers[i].GetAddressOf()));
    device_->CreateRenderTargetView(_backBuffers[i].Get(), nullptr, handle);
    handle.ptr += rtv_descriptor_size_;
  }

  return true;
}

bool Graphic::CreateDSVForFrameBuffer() {
  D3D12_CLEAR_VALUE dsvClearValue;
  dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;
  dsvClearValue.DepthStencil.Depth = 1.0f;
  dsvClearValue.DepthStencil.Stencil = 0;

  CD3DX12_RESOURCE_DESC desc(D3D12_RESOURCE_DIMENSION_TEXTURE2D,
    0,
    frame_buffer_width_,
    frame_buffer_height_,
    1,
    1,
    DXGI_FORMAT_D32_FLOAT,
    1,
    0,
    D3D12_TEXTURE_LAYOUT_UNKNOWN,
    D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

  auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto hr = device_->CreateCommittedResource(&heapProp,
    D3D12_HEAP_FLAG_NONE,
    &desc,
    D3D12_RESOURCE_STATE_DEPTH_WRITE,
    &dsvClearValue,
    IID_PPV_ARGS(depth_stencil_buffer_.GetAddressOf()));

  if (FAILED(hr) || depth_stencil_buffer_ == nullptr) {
    std::cerr << "Failed to create depth stencil buffer." << std::endl;
    return false;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsv_heaps_->GetCPUDescriptorHandleForHeapStart();

  device_->CreateDepthStencilView(depth_stencil_buffer_.Get(), nullptr, dsvHandle);

  return true;
}

bool Graphic::CreateSynchronizationWithGPUObject() {
  auto hr = device_->CreateFence(fence_val_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf()));

  if (FAILED(hr) || fence_ == nullptr) {
    std::cerr << "Failed to create fence." << std::endl;
    return false;
  }

  fence_val_ = 1;

  fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (fence_event_ == nullptr) {
    std::cerr << "Failed to create fence event." << std::endl;
    return false;
  }
  return true;
}
