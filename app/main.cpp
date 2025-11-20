#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <exception>
#include <iostream>
#include <vector>

#include "Application/Application.h"

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

ComPtr<ID3D12Device> device = nullptr;
ComPtr<IDXGIFactory6> dxgiFactory = nullptr;
ComPtr<IDXGISwapChain4> swap_chain = nullptr;

ComPtr<ID3D12CommandAllocator> command_allocator = nullptr;
ComPtr<ID3D12GraphicsCommandList> command_list = nullptr;

ComPtr<ID3D12CommandQueue> command_queue = nullptr;

constexpr int window_width = 1920;
constexpr int window_height = 1080;

void EnableDbeugLayer() {
  ID3D12Debug* debugLayer = nullptr;
  auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

  if (FAILED(result)) {
    throw std::runtime_error("Debug layer failed to initialize");
  }

  debugLayer->EnableDebugLayer();
  debugLayer->Release();
}

int WINAPI wWinMain([[maybe_unused]] HINSTANCE hInstance,
  [[maybe_unused]] HINSTANCE hPrevInstance,
  [[maybe_unused]] PWSTR lpCmdLine,
  [[maybe_unused]] int nCmdShow) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  try {
    Application app(hInstance, window_width, window_height);

// Enable Debug layer
#if _DEBUG
    EnableDbeugLayer();
#endif

    // Create DXGI Factory
    if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory)))) {
      if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory)))) {
        return -1;
      }
    }

    std::vector<IDXGIAdapter*> adapters;
    IDXGIAdapter* tmpAdapter = nullptr;
    for (int i = 0; dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
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

    // Create Device
    D3D_FEATURE_LEVEL levels[] = {
      D3D_FEATURE_LEVEL_12_2,
      D3D_FEATURE_LEVEL_12_1,
      D3D_FEATURE_LEVEL_12_0,
      D3D_FEATURE_LEVEL_11_1,
      D3D_FEATURE_LEVEL_11_0,
    };
    for (auto level : levels) {
      if (SUCCEEDED(D3D12CreateDevice(tmpAdapter, level, IID_PPV_ARGS(&device)))) {
        break;
      }
    }

    // todo: add hresult check
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator));
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator.Get(), nullptr, IID_PPV_ARGS(&command_list));

    D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
    command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    command_queue_desc.NodeMask = 0;
    command_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&command_queue));

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width = window_width;
    swap_chain_desc.Height = window_height;
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
    dxgiFactory->CreateSwapChainForHwnd(
      command_queue.Get(), app.GetHwnd(), &swap_chain_desc, nullptr, nullptr, (swap_chain1.GetAddressOf()));
    swap_chain1.As(&swap_chain);  // todo: check hresult

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap_desc.NodeMask = 0;
    heap_desc.NumDescriptors = 2;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ID3D12DescriptorHeap* rtvHeaps = nullptr;
    device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtvHeaps));

    DXGI_SWAP_CHAIN_DESC swcDesc = {};
    swap_chain->GetDesc(&swcDesc);
    std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
    for (size_t i = 0; i < swcDesc.BufferCount; ++i) {
      swap_chain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&_backBuffers[i]));
      device->CreateRenderTargetView(_backBuffers[i], nullptr, handle);
      handle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    ID3D12Fence* fence = nullptr;
    UINT64 fenceVal = 0;
    device->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

    const std::function<void(float dt)> OnUpdate = [=, &fence, &fenceVal]([[maybe_unused]] float dt) {
      auto bbIdx = swap_chain->GetCurrentBackBufferIndex();

      D3D12_RESOURCE_BARRIER BarrierDesc = {};
      BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      BarrierDesc.Transition.pResource = _backBuffers[bbIdx];
      BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
      BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
      command_list->ResourceBarrier(1, &BarrierDesc);

      auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
      rtvH.ptr += static_cast<ULONG_PTR>(bbIdx * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
      command_list->OMSetRenderTargets(1, &rtvH, false, nullptr);

      float clearColor[] = {1.0f, 1.0f, 0.0f, 1.0f};
      command_list->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

      BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
      BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
      command_list->ResourceBarrier(1, &BarrierDesc);

      command_list->Close();

      ID3D12CommandList* cmdlists[] = {command_list.Get()};
      command_queue->ExecuteCommandLists(1, cmdlists);
      command_queue->Signal(fence, ++fenceVal);

      if (fence->GetCompletedValue() != fenceVal) {
        auto event = CreateEvent(nullptr, false, false, nullptr);
        fence->SetEventOnCompletion(fenceVal, event);
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
      }
      command_allocator->Reset();
      command_list->Reset(command_allocator.Get(), nullptr);

      swap_chain->Present(1, 0);
    };

    const std::function<void(float fdt)> OnFixedUpdate = []([[maybe_unused]] float fdt) { std::cout << "FixedUpdate" << fdt << std::endl; };

    app.Run(OnUpdate, OnFixedUpdate);
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    MessageBoxW(nullptr, L"An exception occurred", L"Error", MB_OK | MB_ICONERROR);
  }
  return 0;
}
