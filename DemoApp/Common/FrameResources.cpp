#include "FrameResources.h"
#include "d3dUtil.h"

FrameResources::FrameResources(ID3D12Device* device, uint32_t objectCount) {
    ThrowIfFailed(device->CreateCommandAllocator(
    D3D12_COMMAND_LIST_TYPE_DIRECT, 
    IID_PPV_ARGS(&commandAllocator)));

    objectConstantBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
}