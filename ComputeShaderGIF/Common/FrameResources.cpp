#include "FrameResources.h"
#include "d3dUtil.h"

namespace FrameUtil {
    FrameResources::FrameResources(ID3D12Device* device, uint32_t objectCount, uint32_t passCount, uint32_t materialCount) {
        ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, 
        IID_PPV_ARGS(&commandAllocator)));

        objectConstantBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
        passConstantBuffer = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
        materialConstantBuffer = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);
    }
}