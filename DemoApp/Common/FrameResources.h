#pragma once

#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <memory>

#include "MathHelper.h"
#include "UploadBuffer.h"

using namespace DirectX;
using namespace Microsoft::WRL;

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
    XMFLOAT2 uv;
};

// 单个物体的物体常量数据(不变的)
struct ObjectConstants {
    XMFLOAT4X4 worldViewProjection = MathHelper::Identity4x4(); 
};

// 为了改进之前CPU和GPU互相等待的低效实现, 让CPU连续处理3帧的绘制命令，
// 并提交至GPU，当GPU处理完一帧命令后，CPU迅速提交命令，始终保证有3帧的命令储配。
// 这样的话，如果CPU提交命令的速度高于GPU处理命令的速度，那GPU就会一直处于忙碌的状态，
// 而空闲的CPU可以处理游戏逻辑、AI、物理模拟等等的计算。我们将每一帧提交的命令称为帧资源，
// 而3帧的命令储备相当于一个3个帧资源元素的环形 数组，随着GPU的进程，CPU不断更新环形数组中的数据元素。
class FrameResources {
public:
    FrameResources(ID3D12Device* device, uint32_t objectCount);
    FrameResources(const FrameResources& rhs) = delete;
    FrameResources& operator=(const FrameResources& rhs) = delete;

    // 在GPU处理完与此命令分配器相关的命令之前，我们不能对它进行重置
    // 所以每一帧都要有它们自己的命令分配器
    ComPtr<ID3D12CommandAllocator> commandAllocator;

    // 在GPU执行完引用此常量缓冲区的命令之前，我们不能对它进行更新
    // 因此每一帧都要有它们自己的常量缓冲区
    std::unique_ptr<UploadBuffer<ObjectConstants>> objectConstantBuffer = nullptr;

    // 通过围栏值将命令标记到此围栏点，这使我们可以检测到GPU是否还在使用这些帧资源
    uint64_t fenceValue = 0;
};