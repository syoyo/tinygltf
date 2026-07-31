#ifndef DXVIEW_VIEWER_GUARD
#define DXVIEW_VIEWER_GUARD

#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <tiny_gltf.h>
#include <wrl.h>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

enum RenderPassType { RENDER_PASS_TYPE_PRESENT = 0, RENDER_PASS_TYPE_COUNT };

struct RenderTarget {
  ComPtr<ID3D12Resource> pTexture;
  D3D12_CPU_DESCRIPTOR_HANDLE viewDescriptor;
};

struct TextureInfo {
  int32_t textureIndex;
  int32_t samplerIndex;
};

struct PBRMetallicRoughness {
  DirectX::XMFLOAT4 baseColorFactor;
  TextureInfo baseColorTexture;
  float metallicFactor;
  float roughnessFactor;
  TextureInfo metallicRoughnessTexture;
};

struct Material {
  std::string name;
  D3D12_BLEND_DESC blendDesc;
  D3D12_RASTERIZER_DESC rasterizerDesc;
  ComPtr<ID3D12Resource> pBuffer;
  void* pBufferData;
  ComPtr<ID3D12DescriptorHeap> pSRVDescriptorHeap;
  ComPtr<ID3D12DescriptorHeap> pSamplerDescriptorHeap;
};

struct Attribute {
  std::string name;
  DXGI_FORMAT format;
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
};

struct Primitive {
  std::vector<Attribute> attributes;
  uint32_t vertexCount;
  D3D12_PRIMITIVE_TOPOLOGY primitiveTopology;
  D3D12_INDEX_BUFFER_VIEW indexBufferView;
  uint32_t indexCount;
  Material* pMaterial;
  ComPtr<ID3D12RootSignature> pRootSignature;
  ComPtr<ID3D12PipelineState> pPipelineState;
};

struct Mesh {
  std::string name;
  std::vector<Primitive> primitives;
};

struct Node {
  DirectX::XMFLOAT4X4 M;
};

struct Camera {
  DirectX::XMFLOAT4X4 V;
  DirectX::XMFLOAT4X4 P;
  DirectX::XMFLOAT4X4 VP;
};

class Viewer {
 public:
  Viewer(HWND window, tinygltf::Model* pModel);

  void update(double deltaTime);
  void render(double deltaTime);

 private:
  void initDirectX(HWND window);
  void buildRenderTargets();
  void buildResources();
  void buildBuffers(std::vector<ComPtr<ID3D12Resource> >* pStagingResources);
  void buildImages(std::vector<ComPtr<ID3D12Resource> >* pStagingResources);
  void buildSamplerDescs();
  void buildMaterials();
  void buildMeshes();
  void buildNodes();

  void drawNode(uint64_t nodeIndex);

 private:
  tinygltf::Model* pModel_;
  ComPtr<IDXGIFactory1> pFactory_;
  ComPtr<IDXGIAdapter1> pAdapter_;
  ComPtr<ID3D12Device> pDevice_;
  ComPtr<ID3D12CommandQueue> pDirectCommandQueue_;
  UINT64 directFenceValue_;
  ComPtr<ID3D12Fence> pDirectFence_;
  ComPtr<ID3D12CommandQueue> pCopyCommandQueue_;
  UINT64 copyFenceValue_;
  ComPtr<ID3D12Fence> pCopyFence_;
  ComPtr<IDXGISwapChain3> pSwapChain_;
  ComPtr<ID3D12Resource> pSwapChainBuffers_[DXVIEW_SWAP_CHAIN_BUFFER_COUNT];
  ComPtr<ID3D12CommandAllocator>
      pDirectCommandAllocators_[DXVIEW_SWAP_CHAIN_BUFFER_COUNT];
  ComPtr<ID3D12GraphicsCommandList> pDirectCommandList_;
  ComPtr<ID3D12CommandAllocator> pCopyCommandAllocator_;
  ComPtr<ID3D12GraphicsCommandList> pCopyCommandList_;
  UINT descriptorIncrementSize_[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
  ComPtr<ID3D12DescriptorHeap>
      pRTVDescriptorHeaps_[DXVIEW_SWAP_CHAIN_BUFFER_COUNT];
  std::vector<RenderTarget> renderTargets_[DXVIEW_SWAP_CHAIN_BUFFER_COUNT];
  std::vector<ComPtr<ID3D12Resource> > pBuffers_;
  std::vector<ComPtr<ID3D12Resource> > pTextures_;
  std::vector<D3D12_SAMPLER_DESC> samplerDescs_;
  std::vector<Material> materials_;
  std::vector<Mesh> meshes_;
  std::vector<ComPtr<ID3D12Resource> > pNodeBuffers_;
  ComPtr<ID3D12Resource> pCameraBuffer_;
};

#endif
