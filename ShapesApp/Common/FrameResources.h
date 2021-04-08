#pragma once

#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <memory>

#include "MathHelper.h"
#include "UploadBuffer.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace FrameUtil {
    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT4 color;
        XMFLOAT2 uv;
    };

    struct RenderItem {
        RenderItem() = default;

        // 描述物体局部空间相对于世界空间的世界矩阵
        // 它定义了物体位于世界空间中的位置、朝向以及大小
        XMFLOAT4X4 world = MathHelper::Identity4x4();

        // 用已更新标志(dirty flag)来表示物体的相关数据已发生改变
        // 这意味着我们此时需要更新常量缓冲区。由于每个FrameResource
        // 中都有一个物体常量缓冲区，所以我们必须对每个FrameResource
        // 都进行更新。即，当我们修改物体数据的时候，应当按
        // NumFramesDirty = numFrameResources进行设置，
        // 从而使每个帧资源都得到更新
        int numFramesDirty = 3;

        // 该索引指向的GPU常量缓冲区对应于当前渲染项中的物体常量缓冲区
        uint32_t objectConstantBufferIndex = -1;

        // 此渲染杂项参与绘制的几何体。注意，绘制一个几何体可能会用到多个渲染项
        MeshGeometry* geometry = nullptr;

        // 图元拓扑
        D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        // DrawIndexedInstanced方法的参数
        uint32_t indexCount = 0;
        uint32_t startIndexLocation = 0;
        int32_t baseVertexLocation = 0;
    };

    // 单个物体的物体常量数据(不变的)
    struct ObjectConstants {
        XMFLOAT4X4 world = MathHelper::Identity4x4(); 
    };

    //单个物体的过程常量数据(每帧变化)
    struct PassConstants {
        XMFLOAT4X4 viewProjection = MathHelper::Identity4x4();
        float time;
    };

    struct MaterialConstants {
        uint32_t materialIndex;
    };

    // 为了改进之前CPU和GPU互相等待的低效实现, 让CPU连续处理3帧的绘制命令，
    // 并提交至GPU，当GPU处理完一帧命令后，CPU迅速提交命令，始终保证有3帧的命令储配。
    // 这样的话，如果CPU提交命令的速度高于GPU处理命令的速度，那GPU就会一直处于忙碌的状态，
    // 而空闲的CPU可以处理游戏逻辑、AI、物理模拟等等的计算。我们将每一帧提交的命令称为帧资源，
    // 而3帧的命令储备相当于一个3个帧资源元素的环形 数组，随着GPU的进程，CPU不断更新环形数组中的数据元素。
    class FrameResources {
    public:
        FrameResources(ID3D12Device* device, uint32_t objectCount, uint32_t passCount, uint32_t materialCount);
        FrameResources(const FrameResources& rhs) = delete;
        FrameResources& operator=(const FrameResources& rhs) = delete;

        // 在GPU处理完与此命令分配器相关的命令之前，我们不能对它进行重置
        // 所以每一帧都要有它们自己的命令分配器
        ComPtr<ID3D12CommandAllocator> commandAllocator;

        // 在GPU执行完引用此常量缓冲区的命令之前，我们不能对它进行更新
        // 因此每一帧都要有它们自己的常量缓冲区
        std::unique_ptr<UploadBuffer<FrameResources::ObjectConstants>> objectConstantBuffer = nullptr;
        std::unique_ptr<UploadBuffer<FrameResources::PassConstants>> passConstantBuffer = nullptr;
        std::unique_ptr<UploadBuffer<FrameResources::MaterialConstants>> materialConstantBuffer = nullptr;

        // 通过围栏值将命令标记到此围栏点，这使我们可以检测到GPU是否还在使用这些帧资源
        uint64_t fenceValue = 0;
    };
}