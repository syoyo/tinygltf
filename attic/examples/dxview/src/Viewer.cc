#include "Viewer.h"

#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <spdlog/spdlog.h>

using namespace DirectX;

namespace {

constexpr bool isPow2(uint64_t value) {
  return (value == 0) ? false : ((value & (value - 1)) == 0);
}

template <typename T>
constexpr T alignPow2(T value, uint64_t alignment) {
  assert(isPow2(alignment));
  return ((value + static_cast<T>(alignment) - 1) &
          ~(static_cast<T>(alignment) - 1));
}

}  // namespace

Viewer::Viewer(HWND window, tinygltf::Model* pModel)
    : pModel_(pModel), directFenceValue_(0), copyFenceValue_(0) {
  initDirectX(window);
  buildRenderTargets();
  buildResources();

  // TODO:
  HRESULT hr = S_OK;

  D3D12_HEAP_PROPERTIES heapProperties = {};
  heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

  D3D12_RESOURCE_DESC resourceDesc = {};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Width =
      alignPow2(sizeof(PBRMetallicRoughness),
                D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc = {1, 0};
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  hr = pDevice_->CreateCommittedResource(
      &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&pCameraBuffer_));
  assert(SUCCEEDED(hr));
}

void Viewer::update(double deltaTime) {
  if (pDirectFence_->GetCompletedValue() < directFenceValue_) {
    auto event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    pDirectFence_->SetEventOnCompletion(directFenceValue_, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
  }

  // TODO:
  void* pData;
  pCameraBuffer_->Map(0, nullptr, &pData);

  auto* pCameraData = static_cast<Camera*>(pData);

  constexpr auto kRadius = 3.0;
  static auto degree = 0.0;
  degree += 10.0 * deltaTime;
  auto radian = degree * XM_PI / 180.0;

  XMMATRIX P =
      XMMatrixPerspectiveFovRH(90.0f * XM_PI / 180.0f, 1.0f, 0.01f, 100.0f);
  XMStoreFloat4x4(&pCameraData->P, XMMatrixTranspose(P));

  XMMATRIX V = XMMatrixLookAtRH(
      XMVectorSet(static_cast<float>(kRadius * cos(radian)), 0.0f,
                  static_cast<float>(kRadius * sin(radian)), 1.0f),
      XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
  XMStoreFloat4x4(&pCameraData->V, XMMatrixTranspose(V));

  XMMATRIX VP = XMMatrixMultiply(V, P);
  XMStoreFloat4x4(&pCameraData->VP, XMMatrixTranspose(VP));
  pCameraBuffer_->Unmap(0, nullptr);
}

void Viewer::render(double deltaTime) {
  const auto kSwapChainBackBufferIndex =
      pSwapChain_->GetCurrentBackBufferIndex();

  auto pDirectCommandAllocator =
      pDirectCommandAllocators_[kSwapChainBackBufferIndex].Get();

  pDirectCommandAllocator->Reset();
  pDirectCommandList_->Reset(pDirectCommandAllocator, nullptr);

  D3D12_VIEWPORT viewport = {};
  viewport.Width = 512.0;
  viewport.Height = 512.0;
  viewport.MaxDepth = 1.0f;

  pDirectCommandList_->RSSetViewports(1, &viewport);

  D3D12_RECT scissorRect = {};
  scissorRect.right = 512;
  scissorRect.bottom = 512;

  pDirectCommandList_->RSSetScissorRects(1, &scissorRect);

  auto& renderTargets = renderTargets_[kSwapChainBackBufferIndex];

  {
    auto& renderTarget = renderTargets[RENDER_PASS_TYPE_PRESENT];
    auto pTexture = renderTarget.pTexture.Get();
    auto rtvDescriptor = renderTarget.viewDescriptor;

    {
      D3D12_RESOURCE_BARRIER resourceBarrier = {};
      resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      resourceBarrier.Transition.pResource = pTexture;
      resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
      resourceBarrier.Transition.StateAfter =
          D3D12_RESOURCE_STATE_RENDER_TARGET;

      pDirectCommandList_->ResourceBarrier(1, &resourceBarrier);
    }

    pDirectCommandList_->OMSetRenderTargets(1, &rtvDescriptor, false, nullptr);
    pDirectCommandList_->ClearRenderTargetView(rtvDescriptor, Colors::Black, 0,
                                               nullptr);

    auto& scene = pModel_->scenes[pModel_->defaultScene];
    for (auto nodeIndex : scene.nodes) {
      drawNode(nodeIndex);
    }

    {
      D3D12_RESOURCE_BARRIER resourceBarrier = {};
      resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      resourceBarrier.Transition.pResource = pTexture;
      resourceBarrier.Transition.StateBefore =
          D3D12_RESOURCE_STATE_RENDER_TARGET;
      resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

      pDirectCommandList_->ResourceBarrier(1, &resourceBarrier);
    }
  }

  pDirectCommandList_->Close();

  ID3D12CommandList* pCommandLists[] = {pDirectCommandList_.Get()};
  pDirectCommandQueue_->ExecuteCommandLists(_countof(pCommandLists),
                                            pCommandLists);
  pDirectCommandQueue_->Signal(pDirectFence_.Get(), ++directFenceValue_);
  pSwapChain_->Present(0, 0);
}

void Viewer::initDirectX(HWND window) {
  HRESULT hr = S_OK;

  {
    UINT flags = 0;

#if _DEBUG
    ComPtr<ID3D12Debug> pDebug;
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug));
    assert(SUCCEEDED(hr));

    if (pDebug) {
      flags |= DXGI_CREATE_FACTORY_DEBUG;
      pDebug->EnableDebugLayer();
    }
#endif

    hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&pFactory_));
    assert(SUCCEEDED(hr));
  }

  {
    UINT i = 0;
    while (DXGI_ERROR_NOT_FOUND != pFactory_->EnumAdapters1(i, &pAdapter_)) {
      DXGI_ADAPTER_DESC1 adapterDesc;
      pAdapter_->GetDesc1(&adapterDesc);

      if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
        continue;
      }

      hr = D3D12CreateDevice(pAdapter_.Get(), D3D_FEATURE_LEVEL_11_0,
                             IID_PPV_ARGS(&pDevice_));
      assert(SUCCEEDED(hr));

      if (pDevice_) {
        break;
      }

      ++i;
    }
  }

  {
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    hr = pDevice_->CreateCommandQueue(&commandQueueDesc,
                                      IID_PPV_ARGS(&pDirectCommandQueue_));
    assert(SUCCEEDED(hr));
  }

  {
    hr = pDevice_->CreateFence(directFenceValue_, D3D12_FENCE_FLAG_NONE,
                               IID_PPV_ARGS(&pDirectFence_));
    assert(SUCCEEDED(hr));
  }

  {
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    hr = pDevice_->CreateCommandQueue(&commandQueueDesc,
                                      IID_PPV_ARGS(&pCopyCommandQueue_));
    assert(SUCCEEDED(hr));
  }

  {
    hr = pDevice_->CreateFence(copyFenceValue_, D3D12_FENCE_FLAG_NONE,
                               IID_PPV_ARGS(&pCopyFence_));
    assert(SUCCEEDED(hr));
  }

  {
    ComPtr<IDXGIOutput> pOutput;
    hr = pAdapter_->EnumOutputs(0, &pOutput);
    assert(SUCCEEDED(hr));

    UINT modeCount = 1;
    DXGI_MODE_DESC modeDesc;
    hr = pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &modeCount,
                                     &modeDesc);
    assert(SUCCEEDED(hr) || hr == DXGI_ERROR_MORE_DATA);

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc = modeDesc;

    // TODO:
    swapChainDesc.BufferDesc.Width = 512;
    swapChainDesc.BufferDesc.Height = 512;

    swapChainDesc.SampleDesc = {1, 0};
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = DXVIEW_SWAP_CHAIN_BUFFER_COUNT;
    swapChainDesc.OutputWindow = window;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain> pSwapChain;
    hr = pFactory_->CreateSwapChain(pDirectCommandQueue_.Get(), &swapChainDesc,
                                    &pSwapChain);
    assert(SUCCEEDED(hr));

    hr = pSwapChain.As(&pSwapChain_);
    assert(SUCCEEDED(hr));
  }

  for (auto i = 0; i != DXVIEW_SWAP_CHAIN_BUFFER_COUNT; ++i) {
    hr = pSwapChain_->GetBuffer(i, IID_PPV_ARGS(&pSwapChainBuffers_[i]));
    assert(SUCCEEDED(hr));
  }

  for (auto i = 0; i != DXVIEW_SWAP_CHAIN_BUFFER_COUNT; ++i) {
    hr = pDevice_->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&pDirectCommandAllocators_[i]));
    assert(SUCCEEDED(hr));
  }

  {
    hr = pDevice_->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, pDirectCommandAllocators_[0].Get(),
        nullptr, IID_PPV_ARGS(&pDirectCommandList_));
    assert(SUCCEEDED(hr));

    hr = pDirectCommandList_->Close();
    assert(SUCCEEDED(hr));
  }

  {
    hr = pDevice_->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&pCopyCommandAllocator_));
    assert(SUCCEEDED(hr));
  }

  {
    hr = pDevice_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY,
                                     pCopyCommandAllocator_.Get(), nullptr,
                                     IID_PPV_ARGS(&pCopyCommandList_));
    assert(SUCCEEDED(hr));

    hr = pCopyCommandList_->Close();
    assert(SUCCEEDED(hr));
  }

  for (auto i = 0; i != D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
    auto descriptorHeapType = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);
    descriptorIncrementSize_[i] =
        pDevice_->GetDescriptorHandleIncrementSize(descriptorHeapType);
  }
}

void Viewer::buildRenderTargets() {
  HRESULT hr = S_OK;

  for (auto i = 0; i != DXVIEW_SWAP_CHAIN_BUFFER_COUNT; ++i) {
    auto& pRTVDescriptorHeap = pRTVDescriptorHeaps_[i];

    {
      D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
      descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
      descriptorHeapDesc.NumDescriptors = 1;
      descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

      hr = pDevice_->CreateDescriptorHeap(&descriptorHeapDesc,
                                          IID_PPV_ARGS(&pRTVDescriptorHeap));
    }

    if (SUCCEEDED(hr)) {
      auto& renderTargets = renderTargets_[i];
      auto RTVDescriptor =
          pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

      for (auto j = 0; j != RENDER_PASS_TYPE_COUNT; ++j) {
        RenderTarget renderTarget = {};

        if (j != RENDER_PASS_TYPE_PRESENT) {
          assert(false);
        } else {
          renderTarget.pTexture = pSwapChainBuffers_[i];
        }

        renderTarget.viewDescriptor = RTVDescriptor;

        pDevice_->CreateRenderTargetView(pSwapChainBuffers_[i].Get(), nullptr,
                                         RTVDescriptor);
        RTVDescriptor.ptr +=
            descriptorIncrementSize_[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];

        renderTargets.push_back(renderTarget);
      }
    }
  }
}

void Viewer::buildResources() {
  HRESULT hr = S_OK;

  hr = pCopyCommandList_->Reset(pCopyCommandAllocator_.Get(), nullptr);
  assert(SUCCEEDED(hr));

  std::vector<ComPtr<ID3D12Resource> > stagingResources;
  stagingResources.reserve(256);

  buildBuffers(&stagingResources);
  buildImages(&stagingResources);

  hr = pCopyCommandList_->Close();
  assert(SUCCEEDED(hr));

  ID3D12CommandList* pCommandLists[] = {pCopyCommandList_.Get()};
  pCopyCommandQueue_->ExecuteCommandLists(_countof(pCommandLists),
                                          pCommandLists);
  pCopyCommandQueue_->Signal(pCopyFence_.Get(), ++copyFenceValue_);

  buildSamplerDescs();
  buildMaterials();
  buildMeshes();
  buildNodes();

  if (pCopyFence_->GetCompletedValue() < copyFenceValue_) {
    auto event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    pCopyFence_->SetEventOnCompletion(copyFenceValue_, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
  }
}

void Viewer::buildBuffers(
    std::vector<ComPtr<ID3D12Resource> >* pStagingResources) {
  HRESULT hr = S_OK;

  for (auto& gltfBuffer : pModel_->buffers) {
    ComPtr<ID3D12Resource> pDstBuffer;
    {
      D3D12_HEAP_PROPERTIES heapProperties = {};
      heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
      heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

      D3D12_RESOURCE_DESC resourceDesc = {};
      resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      resourceDesc.Alignment = 0;
      resourceDesc.Width = gltfBuffer.data.size();
      resourceDesc.Height = 1;
      resourceDesc.DepthOrArraySize = 1;
      resourceDesc.MipLevels = 1;
      resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
      resourceDesc.SampleDesc = {1, 0};
      resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

      hr = pDevice_->CreateCommittedResource(
          &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
          D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pDstBuffer));
      assert(SUCCEEDED(hr));
      pBuffers_.push_back(pDstBuffer);
    }

    ComPtr<ID3D12Resource> pSrcBuffer;
    {
      D3D12_HEAP_PROPERTIES heapProperties = {};
      heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
      heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

      D3D12_RESOURCE_DESC resourceDesc = {};
      resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      resourceDesc.Alignment = 0;
      resourceDesc.Width = gltfBuffer.data.size();
      resourceDesc.Height = 1;
      resourceDesc.DepthOrArraySize = 1;
      resourceDesc.MipLevels = 1;
      resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
      resourceDesc.SampleDesc = {1, 0};
      resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

      hr = pDevice_->CreateCommittedResource(
          &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&pSrcBuffer));
      assert(SUCCEEDED(hr));
      pStagingResources->push_back(pSrcBuffer);

      void* pData;
      hr = pSrcBuffer->Map(0, nullptr, &pData);
      assert(SUCCEEDED(hr));

      memcpy(pData, &gltfBuffer.data[0], gltfBuffer.data.size());
    }

    pCopyCommandList_->CopyBufferRegion(pDstBuffer.Get(), 0, pSrcBuffer.Get(),
                                        0, gltfBuffer.data.size());
  }
}

void Viewer::buildImages(
    std::vector<ComPtr<ID3D12Resource> >* pStagingResources) {
  HRESULT hr = S_OK;

  for (auto& gltfImage : pModel_->images) {
    ComPtr<ID3D12Resource> pDstTexture;
    {
      D3D12_HEAP_PROPERTIES heapProperties = {};
      heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
      heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

      D3D12_RESOURCE_DESC resourceDesc = {};
      resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
      resourceDesc.Alignment = 0;
      resourceDesc.Width = gltfImage.width;
      resourceDesc.Height = gltfImage.height;
      resourceDesc.DepthOrArraySize = 1;
      resourceDesc.MipLevels = 1;
      resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      resourceDesc.SampleDesc = {1, 0};
      resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
      resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

      hr = pDevice_->CreateCommittedResource(
          &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
          D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pDstTexture));
      assert(SUCCEEDED(hr));
      pTextures_.push_back(pDstTexture);
    }

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    UINT rowCount;
    UINT64 rowSize;
    UINT64 size;
    pDevice_->GetCopyableFootprints(&pDstTexture->GetDesc(), 0, 1, 0,
                                    &footprint, &rowCount, &rowSize, &size);

    ComPtr<ID3D12Resource> pSrcBuffer;
    {
      D3D12_HEAP_PROPERTIES heapProperties = {};
      heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
      heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

      D3D12_RESOURCE_DESC resourceDesc = {};
      resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      resourceDesc.Alignment = 0;
      resourceDesc.Width = size;
      resourceDesc.Height = 1;
      resourceDesc.DepthOrArraySize = 1;
      resourceDesc.MipLevels = 1;
      resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
      resourceDesc.SampleDesc = {1, 0};
      resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

      hr = pDevice_->CreateCommittedResource(
          &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&pSrcBuffer));
      assert(SUCCEEDED(hr));
      pStagingResources->push_back(pSrcBuffer);

      void* pData;
      hr = pSrcBuffer->Map(0, nullptr, &pData);
      assert(SUCCEEDED(hr));

      for (auto i = 0; i != rowCount; ++i) {
        memcpy(static_cast<uint8_t*>(pData) + rowSize * i,
               &gltfImage.image[0] + gltfImage.width * gltfImage.component * i,
               gltfImage.width * gltfImage.component);
      }
    }

    D3D12_TEXTURE_COPY_LOCATION dstCopyLocation = {};
    dstCopyLocation.pResource = pDstTexture.Get();
    dstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstCopyLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION srcCopyLocation = {};
    srcCopyLocation.pResource = pSrcBuffer.Get();
    srcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcCopyLocation.PlacedFootprint = footprint;

    pCopyCommandList_->CopyTextureRegion(&dstCopyLocation, 0, 0, 0,
                                         &srcCopyLocation, nullptr);
  }
}

void Viewer::buildSamplerDescs() {
  for (auto& glTFSampler : pModel_->samplers) {
    D3D12_SAMPLER_DESC samplerDesc = {};
    switch (glTFSampler.minFilter) {
      case TINYGLTF_TEXTURE_FILTER_NEAREST:
        if (glTFSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
          samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        else
          samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        break;
      case TINYGLTF_TEXTURE_FILTER_LINEAR:
        if (glTFSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
          samplerDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        else
          samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;
      case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        if (glTFSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
          samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        else
          samplerDesc.Filter = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        break;
      case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        if (glTFSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
          samplerDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        else
          samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;
      case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        if (glTFSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
          samplerDesc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        else
          samplerDesc.Filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        break;
      case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        if (glTFSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
          samplerDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        else
          samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        break;
      default:
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;
    }

    auto toTextureAddressMode = [](int wrap) {
      switch (wrap) {
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
          return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
          return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
          return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        default:
          assert(false);
          return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      }
    };

    samplerDesc.AddressU = toTextureAddressMode(glTFSampler.wrapS);
    samplerDesc.AddressV = toTextureAddressMode(glTFSampler.wrapT);
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MaxLOD = 256;

    samplerDescs_.push_back(samplerDesc);
  }
}

void Viewer::buildMaterials() {
  HRESULT hr = S_OK;

  // Build materials.
  for (auto& glTFMaterial : pModel_->materials) {
    Material material = {};

    // Set a material name.
    material.name = glTFMaterial.name;

    // Set a blend desc.
    auto& blendDesc = material.blendDesc;
    if (glTFMaterial.alphaMode == "BLEND") {
      blendDesc.RenderTarget[0].BlendEnable = true;
      blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
      blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
      blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
      blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
      blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
      blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
      blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    } else if (glTFMaterial.alphaMode == "MASK") {
      assert(false);
    }

    blendDesc.RenderTarget[0].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;

    // Set a rasterizer desc.
    auto& rasterizerDesc = material.rasterizerDesc;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    if (glTFMaterial.doubleSided) {
      rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    } else {
      rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    }

    rasterizerDesc.FrontCounterClockwise = true;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster =
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    auto& pBuffer = material.pBuffer;
    auto& pBufferData = material.pBufferData;
    {
      D3D12_HEAP_PROPERTIES heapProperties = {};
      heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
      heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

      D3D12_RESOURCE_DESC resourceDesc = {};
      resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      resourceDesc.Width =
          alignPow2(sizeof(PBRMetallicRoughness),
                    D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
      resourceDesc.Height = 1;
      resourceDesc.DepthOrArraySize = 1;
      resourceDesc.MipLevels = 1;
      resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
      resourceDesc.SampleDesc = {1, 0};
      resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

      hr = pDevice_->CreateCommittedResource(
          &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pBuffer));
      assert(SUCCEEDED(hr));

      hr = pBuffer->Map(0, nullptr, &pBufferData);
      assert(SUCCEEDED(hr));
    }

    auto& pSRVDescriptorHeap = material.pSRVDescriptorHeap;
    {
      D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};

      descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      descriptorHeapDesc.NumDescriptors = 5;
      descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

      hr = pDevice_->CreateDescriptorHeap(&descriptorHeapDesc,
                                          IID_PPV_ARGS(&pSRVDescriptorHeap));
      assert(SUCCEEDED(hr));
    }

    auto& pSamplerDescriptorHeap = material.pSamplerDescriptorHeap;
    {
      D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};

      descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
      descriptorHeapDesc.NumDescriptors = 5;
      descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

      hr = pDevice_->CreateDescriptorHeap(
          &descriptorHeapDesc, IID_PPV_ARGS(&pSamplerDescriptorHeap));
      assert(SUCCEEDED(hr));
    }

    auto& glTFPBRMetallicRoughness = glTFMaterial.pbrMetallicRoughness;
    {
      auto pPBRMetallicRoughness =
          static_cast<PBRMetallicRoughness*>(pBufferData);

      auto& baseColorFactor = pPBRMetallicRoughness->baseColorFactor;
      
      baseColorFactor.x =
          static_cast<float>(glTFPBRMetallicRoughness.baseColorFactor[0]);
      baseColorFactor.y =
          static_cast<float>(glTFPBRMetallicRoughness.baseColorFactor[1]);
      baseColorFactor.z =
          static_cast<float>(glTFPBRMetallicRoughness.baseColorFactor[2]);
      baseColorFactor.w =
          static_cast<float>(glTFPBRMetallicRoughness.baseColorFactor[3]);

      auto& glTFBaseColorTexture = glTFPBRMetallicRoughness.baseColorTexture;
      auto& baseColorTexture = pPBRMetallicRoughness->baseColorTexture;
      if (glTFBaseColorTexture.index >= 0) {
        baseColorTexture.textureIndex = 0;
        baseColorTexture.samplerIndex = 0;
      } else {
        baseColorTexture.textureIndex = -1;
        baseColorTexture.samplerIndex = -1;
      }

      pPBRMetallicRoughness->metallicFactor =
          static_cast<float>(glTFPBRMetallicRoughness.metallicFactor);
      pPBRMetallicRoughness->roughnessFactor =
          static_cast<float>(glTFPBRMetallicRoughness.roughnessFactor);

      auto& glTFMetallicRoughnessTexture =
          glTFPBRMetallicRoughness.metallicRoughnessTexture;
      auto& metallicRoughnessTexture =
          pPBRMetallicRoughness->metallicRoughnessTexture;
      if (glTFMetallicRoughnessTexture.index >= 0) {
        metallicRoughnessTexture.textureIndex = 1;
        metallicRoughnessTexture.samplerIndex = 1;
      } else {
        metallicRoughnessTexture.textureIndex = -1;
        metallicRoughnessTexture.samplerIndex = -1;
      }
    }

    auto srvDescriptor =
        pSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    auto samplerDescriptor =
        pSamplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    auto& glTFBaseColorTexture = glTFPBRMetallicRoughness.baseColorTexture;
    if (glTFBaseColorTexture.index >= 0) {
      auto& glTFTexture = pModel_->textures[glTFBaseColorTexture.index];

      auto pTexture = pTextures_[glTFTexture.source].Get();
      pDevice_->CreateShaderResourceView(pTexture, nullptr, srvDescriptor);

      auto& samplerDesc = samplerDescs_[glTFTexture.sampler];
      pDevice_->CreateSampler(&samplerDesc, samplerDescriptor);
    }

    srvDescriptor.ptr +=
        descriptorIncrementSize_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
    samplerDescriptor.ptr +=
        descriptorIncrementSize_[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER];

    auto& glTFMetallicRoughnessTexture =
        glTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture;
    if (glTFMetallicRoughnessTexture.index >= 0) {
      auto& glTFTexture = pModel_->textures[glTFMetallicRoughnessTexture.index];
      auto pTexture = pTextures_[glTFTexture.source].Get();

      pDevice_->CreateShaderResourceView(pTexture, nullptr, srvDescriptor);

      auto& samplerDesc = samplerDescs_[glTFTexture.sampler];
      pDevice_->CreateSampler(&samplerDesc, samplerDescriptor);
    }
    materials_.push_back(material);
  }
}

void Viewer::buildMeshes() {
  HRESULT hr = S_OK;

  for (auto& glTFMesh : pModel_->meshes) {
    Mesh mesh = {};
    mesh.name = glTFMesh.name;

    auto& primitives = mesh.primitives;
    for (auto& glTFPrimitive : glTFMesh.primitives) {
      Primitive primitive = {};

      auto& attributes = primitive.attributes;
      for (auto& [attributeName, accessorIndex] : glTFPrimitive.attributes) {
        const auto& glTFAccessor = pModel_->accessors[accessorIndex];
        const auto& glTFBufferView =
            pModel_->bufferViews[glTFAccessor.bufferView];
        const auto& glTFBuffer = pModel_->buffers[glTFBufferView.buffer];

        Attribute attribute = {};
        attribute.name = attributeName;

        switch (glTFAccessor.type) {
          case TINYGLTF_TYPE_VEC2:
            attribute.format = DXGI_FORMAT_R32G32_FLOAT;
            break;
          case TINYGLTF_TYPE_VEC3:
            attribute.format = DXGI_FORMAT_R32G32B32_FLOAT;
            break;
          case TINYGLTF_TYPE_VEC4:
            attribute.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            break;
        }

        attribute.vertexBufferView.BufferLocation =
            pBuffers_[glTFBufferView.buffer]->GetGPUVirtualAddress() +
            glTFBufferView.byteOffset + glTFAccessor.byteOffset;
        attribute.vertexBufferView.SizeInBytes = static_cast<UINT>(
            glTFBufferView.byteLength - glTFAccessor.byteOffset);
        attribute.vertexBufferView.StrideInBytes =
            glTFAccessor.ByteStride(glTFBufferView);

        attributes.emplace_back(attribute);

        if (attributeName == "POSITION") {
          primitive.vertexCount = static_cast<uint32_t>(glTFAccessor.count);
        }
      }

      auto& primitiveTopology = primitive.primitiveTopology;
      switch (glTFPrimitive.mode) {
        case TINYGLTF_MODE_POINTS:
          primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
          break;
        case TINYGLTF_MODE_LINE:
          primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
          break;
        case TINYGLTF_MODE_LINE_STRIP:
          primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
          break;
        case TINYGLTF_MODE_TRIANGLES:
          primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
          break;
        case TINYGLTF_MODE_TRIANGLE_STRIP:
          primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
          break;
        default:
          assert(false);
      }

      if (glTFPrimitive.indices >= 0) {
        const auto& glTFAccessor = pModel_->accessors[glTFPrimitive.indices];
        const auto& glTFBufferView =
            pModel_->bufferViews[glTFAccessor.bufferView];
        const auto& glTFBuffer = pModel_->buffers[glTFBufferView.buffer];

        auto& indexBufferView = primitive.indexBufferView;
        indexBufferView.BufferLocation =
            pBuffers_[glTFBufferView.buffer]->GetGPUVirtualAddress() +
            glTFBufferView.byteOffset + glTFAccessor.byteOffset;
        indexBufferView.SizeInBytes = static_cast<UINT>(
            glTFBufferView.byteLength - glTFAccessor.byteOffset);

        switch (glTFAccessor.componentType) {
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            indexBufferView.Format = DXGI_FORMAT_R8_UINT;
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            indexBufferView.Format = DXGI_FORMAT_R16_UINT;
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            indexBufferView.Format = DXGI_FORMAT_R32_UINT;
            break;
        }

        auto& indexCount = primitive.indexCount;
        indexCount = static_cast<uint32_t>(glTFAccessor.count);
      }

      auto buildDefines = [](const std::vector<Attribute>& attributes) {
        std::vector<D3D_SHADER_MACRO> defines;

        for (auto& attribute : attributes) {
          if (attribute.name == "NORMAL")
            defines.push_back({"HAS_NORMAL", "1"});
          else if (attribute.name == "TANGENT")
            defines.push_back({"HAS_TANGENT", "1"});
          else if (attribute.name == "TEXCOORD_0")
            defines.push_back({"HAS_TEXCOORD_0", "1"});
        }

        defines.push_back({nullptr, nullptr});

        return defines;
      };

      auto compileShaderFromFile = [](LPCWSTR pFilePath,
                                      D3D_SHADER_MACRO* pDefines,
                                      LPCSTR pTarget, ID3DBlob** ppShader) {
        HRESULT hr = S_OK;

        {
          UINT flags = 0;

#if _DEBUG
          flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

          ComPtr<ID3DBlob> pError;
          hr = D3DCompileFromFile(pFilePath, pDefines,
                                  D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
                                  pTarget, flags, 0, ppShader, &pError);

          if (pError) {
            spdlog::error("{}", static_cast<char*>(pError->GetBufferPointer()));
          }
          assert(SUCCEEDED(hr));
        }

        return hr;
      };

      auto createRootSignature =
          [this](D3D12_ROOT_SIGNATURE_DESC* pRootSignatureDesc,
                 ID3D12RootSignature** ppRootSignature) {
            HRESULT hr = S_OK;

            ComPtr<ID3DBlob> pSerializeRootSignature;
            ComPtr<ID3DBlob> pError;

            {
              hr = D3D12SerializeRootSignature(
                  pRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                  pSerializeRootSignature.GetAddressOf(),
                  pError.GetAddressOf());

              if (pError) {
                spdlog::error("{}",
                              static_cast<char*>(pError->GetBufferPointer()));
              }
              assert(SUCCEEDED(hr));
            }

            if (SUCCEEDED(hr)) {
              hr = pDevice_->CreateRootSignature(
                  0, pSerializeRootSignature->GetBufferPointer(),
                  pSerializeRootSignature->GetBufferSize(),
                  IID_PPV_ARGS(ppRootSignature));
              assert(SUCCEEDED(hr));
            }

            return hr;
          };

      auto buildInputElementDescs =
          [](const std::vector<Attribute>& attributes) {
            std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

            for (auto& attribute : attributes) {
              D3D12_INPUT_ELEMENT_DESC inputElementDesc = {};
              inputElementDesc.SemanticName = &attribute.name[0];
              inputElementDesc.Format = attribute.format;

              // TODO: Need to parse semantic name and index from attribute name.
              if (attribute.name == "TEXCOORD_0") {
                inputElementDesc.SemanticName = "TEXCOORD_";
                inputElementDesc.SemanticIndex = 0;
              }

              inputElementDesc.InputSlot =
                  static_cast<UINT>(inputElementDescs.size());
              inputElementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
              inputElementDesc.InputSlotClass =
                  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

              inputElementDescs.push_back(inputElementDesc);
            }

            return inputElementDescs;
          };

      if (glTFPrimitive.material >= 0) {
        primitive.pMaterial = &materials_[glTFPrimitive.material];

        auto& pRootSignature = primitive.pRootSignature;
        {
          D3D12_DESCRIPTOR_RANGE SRVDescriptorRange = {};

          SRVDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
          SRVDescriptorRange.NumDescriptors = 5;

          D3D12_DESCRIPTOR_RANGE SamplerDescriptorRange = {};

          SamplerDescriptorRange.RangeType =
              D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
          SamplerDescriptorRange.NumDescriptors = 5;

          D3D12_ROOT_PARAMETER rootParams[5] = {
              {D3D12_ROOT_PARAMETER_TYPE_CBV,
               {0, 0},
               D3D12_SHADER_VISIBILITY_VERTEX},
              {D3D12_ROOT_PARAMETER_TYPE_CBV,
               {1, 0},
               D3D12_SHADER_VISIBILITY_VERTEX},
              {D3D12_ROOT_PARAMETER_TYPE_CBV,
               {2, 0},
               D3D12_SHADER_VISIBILITY_PIXEL},
              {D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
               {1, &SRVDescriptorRange},
               D3D12_SHADER_VISIBILITY_PIXEL},
              {D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
               {1, &SamplerDescriptorRange},
               D3D12_SHADER_VISIBILITY_PIXEL}};

          // TEMP
          rootParams[0].Descriptor.RegisterSpace = 0;
          rootParams[1].Descriptor.RegisterSpace = 0;
          rootParams[2].Descriptor.RegisterSpace = 0;

          D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
          rootSignatureDesc.NumParameters = 5;
          rootSignatureDesc.pParameters = &rootParams[0];
          rootSignatureDesc.Flags =
              D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

          hr = createRootSignature(&rootSignatureDesc, &pRootSignature);
          assert(SUCCEEDED(hr));
        }

        {
          auto defines = buildDefines(attributes);
          auto pDefinces = defines.empty() ? nullptr : &defines[0];

          ComPtr<ID3DBlob> pVS;
          hr = compileShaderFromFile(DXVIEW_RES_DIR "/primitive.hlsl",
                                     pDefinces, "vs_5_1", &pVS);
          assert(SUCCEEDED(hr));

          ComPtr<ID3DBlob> pPS;
          hr = compileShaderFromFile(DXVIEW_RES_DIR "/lighting.hlsl", pDefinces,
                                     "ps_5_1", &pPS);

          auto inputElementDescs = buildInputElementDescs(attributes);

          D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
          pipelineStateDesc.pRootSignature = pRootSignature.Get();
          pipelineStateDesc.VS = {pVS->GetBufferPointer(),
                                  pVS->GetBufferSize()};
          pipelineStateDesc.PS = {pPS->GetBufferPointer(),
                                  pPS->GetBufferSize()};
          pipelineStateDesc.BlendState = primitive.pMaterial->blendDesc;
          pipelineStateDesc.SampleMask = UINT_MAX;
          pipelineStateDesc.RasterizerState =
              primitive.pMaterial->rasterizerDesc;
          pipelineStateDesc.InputLayout = {
              inputElementDescs.data(),
              static_cast<UINT>(inputElementDescs.size())};

          switch (primitive.primitiveTopology) {
            case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
              pipelineStateDesc.PrimitiveTopologyType =
                  D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
              break;
            case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
            case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
              pipelineStateDesc.PrimitiveTopologyType =
                  D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
              break;
            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
              pipelineStateDesc.PrimitiveTopologyType =
                  D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
              break;
            default:
              assert(false);
          }

          pipelineStateDesc.NumRenderTargets = 1;
          pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
          pipelineStateDesc.SampleDesc = {1, 0};

          auto& pPipelineState = primitive.pPipelineState;
          hr = pDevice_->CreateGraphicsPipelineState(
              &pipelineStateDesc, IID_PPV_ARGS(&pPipelineState));
          assert(SUCCEEDED(hr));
        }
      } else {
        auto& pRootSignature = primitive.pRootSignature;
        {
          D3D12_ROOT_PARAMETER rootParams[2] = {
              {D3D12_ROOT_PARAMETER_TYPE_CBV,
               {0, 0},
               D3D12_SHADER_VISIBILITY_VERTEX},
              {D3D12_ROOT_PARAMETER_TYPE_CBV,
               {1, 0},
               D3D12_SHADER_VISIBILITY_VERTEX}};

          // TEMP
          rootParams[0].Descriptor.RegisterSpace = 0;
          rootParams[1].Descriptor.RegisterSpace = 0;

          D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
          rootSignatureDesc.NumParameters = _countof(rootParams);
          rootSignatureDesc.pParameters = &rootParams[0];
          rootSignatureDesc.Flags =
              D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

          hr = createRootSignature(&rootSignatureDesc, &pRootSignature);
          assert(SUCCEEDED(hr));
        }

        {
          auto defines = buildDefines(attributes);
          auto pDefinces = defines.empty() ? nullptr : &defines[0];

          ComPtr<ID3DBlob> pVS;
          hr = compileShaderFromFile(DXVIEW_RES_DIR "/primitive.hlsl",
                                     pDefinces, "vs_5_1", &pVS);
          assert(SUCCEEDED(hr));

          ComPtr<ID3DBlob> pPS;
          hr = compileShaderFromFile(DXVIEW_RES_DIR "/gray.hlsl", pDefinces,
                                     "ps_5_1", &pPS);

          auto inputElementDescs = buildInputElementDescs(attributes);

          D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
          pipelineStateDesc.pRootSignature = pRootSignature.Get();
          pipelineStateDesc.VS = {pVS->GetBufferPointer(),
                                  pVS->GetBufferSize()};
          pipelineStateDesc.PS = {pPS->GetBufferPointer(),
                                  pPS->GetBufferSize()};
          pipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask =
              D3D12_COLOR_WRITE_ENABLE_ALL;
          pipelineStateDesc.SampleMask = UINT_MAX;
          pipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
          pipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
          pipelineStateDesc.RasterizerState.FrontCounterClockwise = true;
          pipelineStateDesc.InputLayout = {
              inputElementDescs.data(),
              static_cast<UINT>(inputElementDescs.size())};

          switch (primitive.primitiveTopology) {
            case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
              pipelineStateDesc.PrimitiveTopologyType =
                  D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
              break;
            case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
            case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
              pipelineStateDesc.PrimitiveTopologyType =
                  D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
              break;
            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
              pipelineStateDesc.PrimitiveTopologyType =
                  D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
              break;
            default:
              assert(false);
          }

          pipelineStateDesc.NumRenderTargets = 1;
          pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
          pipelineStateDesc.SampleDesc = {1, 0};

          auto& pPipelineState = primitive.pPipelineState;
          hr = pDevice_->CreateGraphicsPipelineState(
              &pipelineStateDesc, IID_PPV_ARGS(&pPipelineState));
          assert(SUCCEEDED(hr));
        }
      }
      primitives.push_back(primitive);
    }
    meshes_.push_back(mesh);
  }
}

void Viewer::buildNodes() {
  HRESULT hr = S_OK;

  for (auto& glTFNode : pModel_->nodes) {
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width =
        alignPow2(sizeof(PBRMetallicRoughness),
                  D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc = {1, 0};
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> pBuffer;
    hr = pDevice_->CreateCommittedResource(
        &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pBuffer));
    assert(SUCCEEDED(hr));

    void* pData;
    hr = pBuffer->Map(0, nullptr, &pData);
    assert(SUCCEEDED(hr));

    if (glTFNode.matrix.empty()) {
      XMStoreFloat4x4(static_cast<DirectX::XMFLOAT4X4*>(pData),
                      XMMatrixIdentity());
    } else {
      float* pElement = static_cast<float*>(pData);
      for (auto value : glTFNode.matrix) {
        *pElement = static_cast<float>(value);
        ++pElement;
      }
    }

    pNodeBuffers_.push_back(pBuffer);
  }
}

void Viewer::drawNode(uint64_t nodeIndex) {
  const auto& glTFNode = pModel_->nodes[nodeIndex];

  if (glTFNode.mesh >= 0) {
    const auto& mesh = meshes_[glTFNode.mesh];

    for (auto& primitive : mesh.primitives) {
      pDirectCommandList_->SetGraphicsRootSignature(
          primitive.pRootSignature.Get());
      pDirectCommandList_->SetPipelineState(primitive.pPipelineState.Get());
      pDirectCommandList_->IASetPrimitiveTopology(primitive.primitiveTopology);

      for (auto i = 0; i != primitive.attributes.size(); ++i) {
        pDirectCommandList_->IASetVertexBuffers(
            i, 1, &primitive.attributes[i].vertexBufferView);
      }

      ID3D12DescriptorHeap* pDescriptorHeaps[] = {
          primitive.pMaterial->pSRVDescriptorHeap.Get(),
          primitive.pMaterial->pSamplerDescriptorHeap.Get()};
      pDirectCommandList_->SetDescriptorHeaps(_countof(pDescriptorHeaps),
                                              pDescriptorHeaps);
      
      pDirectCommandList_->SetGraphicsRootConstantBufferView(
          0, pCameraBuffer_->GetGPUVirtualAddress());
      pDirectCommandList_->SetGraphicsRootConstantBufferView(
          1, pNodeBuffers_[nodeIndex]->GetGPUVirtualAddress());
      pDirectCommandList_->SetGraphicsRootConstantBufferView(
          2, primitive.pMaterial->pBuffer->GetGPUVirtualAddress());
      pDirectCommandList_->SetGraphicsRootDescriptorTable(
          3, primitive.pMaterial->pSRVDescriptorHeap
                 ->GetGPUDescriptorHandleForHeapStart());
      pDirectCommandList_->SetGraphicsRootDescriptorTable(
          4, primitive.pMaterial->pSamplerDescriptorHeap
                 ->GetGPUDescriptorHandleForHeapStart());

      if (primitive.indexCount) {
        pDirectCommandList_->IASetIndexBuffer(&primitive.indexBufferView);
        pDirectCommandList_->DrawIndexedInstanced(primitive.indexCount, 1, 0, 0,
                                                  0);
      } else {
        pDirectCommandList_->DrawInstanced(primitive.vertexCount, 1, 0, 0);
      }
    }
  }

  for (auto childNodeIndex : glTFNode.children) {
    drawNode(childNodeIndex);
  }
}
