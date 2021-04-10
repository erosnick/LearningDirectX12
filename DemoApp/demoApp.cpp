#include "demoApp.h"
#include "Common/d3dUtil.h"
#include "Common/WICUtils.h"
#include <DirectXColors.h>

#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

demoApp::demoApp(HINSTANCE inInstance, const uint32_t inWindowWidth, const uint32_t inWindowHeight)
: d3dApp(inInstance, inWindowWidth, inWindowHeight) {

}

demoApp::~demoApp() {
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT demoApp::msgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
	{
		return true;
	}
	
	if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
	{
		return true;
	}

	return d3dApp::msgProc(hwnd, msg, wParam, lParam);
}

bool demoApp::initialize() {
    if(!d3dApp::initialize()) {
        return false;
    }

    createCBVSRVDescriptorHeaps();
    createFrameResources();
    loadResources();
    createShaderResourceView();
    createConstantBufferViews();
    createRootSignature();
    createShadersAndInputLayout();
    createBoxGeometry();
    createPipelineStateOjbect();

    initImGUI();

	return true;
}

void demoApp::onResize() {
    d3dApp::onResize();

	ImGui_ImplDX12_InvalidateDeviceObjects();
	ImGui_ImplDX12_CreateDeviceObjects();

    // 若用户调整窗口尺寸，则更新纵横比并重新计算投影矩阵
    XMMATRIX newProjection = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspectRatio(), 1.0f, 1000.0f);

    float nearZ = 1.0f;
    float farZ = 1000.0f;
    float fov = 0.25f * MathHelper::Pi;

    float halfFovTan = tanf(fov / 2);

    newProjection.r[0] = XMVectorSet(1.0f / (halfFovTan * aspectRatio()), 0.0f, 0.0f, 0.0f);
    newProjection.r[1] = XMVectorSet(0.0f, 1.0f / halfFovTan, 0.0f, 0.0f);
    newProjection.r[2] = XMVectorSet(0.0f, 0.0f, (farZ) / (farZ - nearZ), 1.0f);
    newProjection.r[3] = XMVectorSet(0.0f, 0.0f, -farZ * nearZ / (farZ - nearZ), 0.0f);

    XMStoreFloat4x4(&projection, newProjection);
}

void demoApp::update(float delta) {
    frameResourceSync();

    // 将球坐标转换为笛卡尔坐标
    float x = radius * sinf(phi) * cosf(theta);
    float z = radius * sinf(phi) * sinf(theta);
    float y = radius * cosf(phi);

    // 构建观察矩阵
    XMVECTOR position = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0);

    XMVECTOR forward = XMVectorSubtract(target, position);
    forward = XMVector3Normalize(forward);

    XMVECTOR right = XMVector3Cross(up, forward);

    right = XMVector3Normalize(right);

    up = XMVector3Cross(forward, right);

    XMFLOAT4 tempRight;
    XMStoreFloat4(&tempRight, right);

    XMFLOAT4 tempUp;
    XMStoreFloat4(&tempUp, up);

    XMFLOAT4 tempForward;
    XMStoreFloat4(&tempForward, forward);

    XMMATRIX newView = XMMatrixIdentity();
    // XMMATRIX newView = XMMatrixLookAtLH(position, target, up);

    newView.r[0] = XMVectorSet(tempRight.x, tempUp.x, tempForward.x, 0.0f);
    newView.r[1] = XMVectorSet(tempRight.y, tempUp.y, tempForward.y, 0.0f);
    newView.r[2] = XMVectorSet(tempRight.z, tempUp.z, tempForward.z, 0.0f);
    
    float tx = -XMVectorGetX(XMVector3Dot(position, right));
    float ty = -XMVectorGetX(XMVector3Dot(position, up));
    float tz = -XMVectorGetX(XMVector3Dot(position, forward));

    newView.r[3] = XMVectorSet(tx, ty, tz, 1.0f);

    XMStoreFloat4x4(&view, newView);

    world.m[3][0] = 0.0f;
    world.m[3][1] = 0.0f;
    world.m[3][2] = 0.0f;

    XMMATRIX newWorld1 = XMLoadFloat4x4(&world);

    world.m[3][0] = 2.0f;
    world.m[3][1] = 0.0f;
    world.m[3][2] = 0.0f;

    XMMATRIX newWorld2 = XMLoadFloat4x4(&world);

    XMMATRIX newProjection = XMLoadFloat4x4(&projection);
    XMMATRIX newWorldViewProjection = newWorld1 * newView * newProjection;

    // 用当前最新的worldViewProject矩阵来更新常量缓冲区
    ObjectConstants objectConstants;
    XMStoreFloat4x4(&objectConstants.worldViewProjection, XMMatrixTranspose(newWorldViewProjection));
    currentFrameResource->objectConstantBuffer->CopyData(0, objectConstants);

    // newWorldViewProjection = newWorld2 * newView * newProjection;

    // XMStoreFloat4x4(&objectConstants.worldViewProjection, XMMatrixTranspose(newWorldViewProjection));
    // currentFrameResource->objectConstantBuffer->CopyData(1, objectConstants);

    updateImGui();
	buildImGuiWidgets();
}

void demoApp::draw(float delta) {
    // 重复使用记录命令的相关内存
    // 只有当与GPU关联的命令列表执行完成时，我们才能将其重置
    commandAllocator = currentFrameResource->commandAllocator;
    ThrowIfFailed(commandAllocator->Reset());

    // 在通过ExecuteCommandList方法将某个命令列表加入命令队列后，
    // 我们便可以重置该命令列表。以此来复用命令列表及其内存
    ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineStateObject.Get()));

    // 对资源的状态进行转换，将资源从呈现状态转换为渲染目标状态
    commandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
        currentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET));

    // 设置视口和裁剪矩形。它们需要随着命令列表的重置而重置
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    // 清除后台缓冲区和深度缓冲区
    commandList->ClearRenderTargetView(currentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
    commandList->ClearDepthStencilView(depthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // 指定要渲染的缓冲区
    commandList->OMSetRenderTargets(1, &currentBackBufferView(), true, &depthStencilView());

    commandList->SetDescriptorHeaps(1, CBVDescriptorHeap.GetAddressOf());

    commandList->SetGraphicsRootSignature(rootSignature.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &boxGeometry->VertexBufferView());
    commandList->IASetIndexBuffer(&boxGeometry->IndexBufferView());

    uint32_t offset = frameResourcesCount * objectCount + 1;

    CD3DX12_GPU_DESCRIPTOR_HANDLE SRVDescriptorHandle(CBVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), offset, CBVSRVUAVDescriptorSize);

    for (int32_t objectIndex = 0; objectIndex < objectCount; objectIndex++) {
        CD3DX12_GPU_DESCRIPTOR_HANDLE CBVDescriptorHeapHandle(CBVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

        int32_t descriptorHeapIndex = currentFrameIndex * objectCount + objectIndex;

        CBVDescriptorHeapHandle.Offset(descriptorHeapIndex, CBVSRVUAVDescriptorSize);

        commandList->SetGraphicsRootDescriptorTable(0, CBVDescriptorHeapHandle);
        commandList->SetGraphicsRootDescriptorTable(2, SRVDescriptorHandle);

        commandList->DrawIndexedInstanced(boxGeometry->DrawArgs["Box"].IndexCount, 1, 0, 0, 0);
    }

    commandList->SetDescriptorHeaps(1, SRVDescriptorHeap.GetAddressOf());
    commandList->SetGraphicsRootDescriptorTable(3, SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    renderImGui();

    // 再次将资源状态进行转换，将资源从渲染目标状态转换回呈现状态
    commandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(
        currentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT));

    // 完成命令的记录
    ThrowIfFailed(commandList->Close());

    // 将待执行的命令队列加入命令列表
    ID3D12CommandList* commandLists[] = {commandList.Get()};
    commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // 交换后台缓冲区和前台缓冲区
    ThrowIfFailed(swapChain->Present(0, 0));

    currentFrameBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

    prepareFrameResourceSync();
}

// Convenience overrides for handling mouse input.
void demoApp::onMouseDown(WPARAM btnState, int32_t x, int32_t y) {
    lastMousePosition.x = x;
    lastMousePosition.y = y;

    SetCapture(mainWindow);
}

void demoApp::onMouseUp(WPARAM btnState, int32_t x, int32_t y) {
    ReleaseCapture();
}

void demoApp::onMouseMove(WPARAM btnState, int32_t x, int32_t y) {
    if ((btnState & MK_LBUTTON) != 0) {
        // 根据鼠标的移动距离计算旋转角度，并令每个像素都按此
        // 角度的1 / 4 旋转
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - lastMousePosition.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - lastMousePosition.y));

        // 根据鼠标的输入来更新摄像机绕立方体旋转的角度
        theta += dx;
        phi += dy;

        // 限制角度phi的范围
        phi = MathHelper::Clamp(phi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0) {
        // 使场景中的每个像素按鼠标移动距离的0.005倍进行缩放
        float dx = 0.005f * static_cast<float>(x - lastMousePosition.x);
        float dy = 0.005f * static_cast<float>(y - lastMousePosition.y);

        // 根据鼠标的输入更新摄像机的可视范围半径
        radius += dx - dy;

        // 限制可视半径的范围
        radius = MathHelper::Clamp(radius, 3.0f, 15.0f);
    }

    lastMousePosition.x = x;
    lastMousePosition.y = y;
}

void demoApp::onKeyDown(WPARAM btnState) {

}

void demoApp::onKeyUp(WPARAM btnState) {

}

void demoApp::createCBVSRVDescriptorHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC CBVDescriptorHeapDesc;
    CBVDescriptorHeapDesc.NumDescriptors = frameBackBufferCount * objectCount + 2;
    CBVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    CBVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    CBVDescriptorHeapDesc.NodeMask = 0;
    ThrowIfFailed(device->CreateDescriptorHeap(&CBVDescriptorHeapDesc, IID_PPV_ARGS(CBVDescriptorHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC SRVDescriptorHeapDesc;
    SRVDescriptorHeapDesc.NumDescriptors = 1;
    SRVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    SRVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    SRVDescriptorHeapDesc.NodeMask = 0;
    ThrowIfFailed(device->CreateDescriptorHeap(&SRVDescriptorHeapDesc, IID_PPV_ARGS(SRVDescriptorHeap.GetAddressOf())));
}

void demoApp::createFrameResources() {
    for (int frameResourceIndex = 0; frameResourceIndex < frameBackBufferCount; frameResourceIndex++) {
        frameResources.push_back(std::make_unique<FrameResources>(device.Get(), objectCount));
    }
}

void demoApp::createConstantBufferViews() {

    UINT constantBufferSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    for (uint32_t frameResourcesIndex = 0; frameResourcesIndex < frameResourcesCount; frameResourcesIndex++) {
        
        for (int32_t objectIndex = 0; objectIndex < objectCount; objectIndex++) {
            auto& objectConstantBuffer = frameResources[frameResourcesIndex]->objectConstantBuffer;
            
            D3D12_GPU_VIRTUAL_ADDRESS constantBufferAddress = objectConstantBuffer->Resource()->GetGPUVirtualAddress();

            // 偏移到常量缓冲区中第i个物体所对应的常量数据
            constantBufferAddress += objectIndex * constantBufferSize;

            CD3DX12_CPU_DESCRIPTOR_HANDLE CBVDescriptorHandle(CBVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

            int32_t descriptorHeapIndex = frameResourcesIndex * objectCount + objectIndex;

            CBVDescriptorHandle.Offset(descriptorHeapIndex, CBVSRVUAVDescriptorSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc;
            constantBufferViewDesc.BufferLocation = constantBufferAddress;
            constantBufferViewDesc.SizeInBytes = constantBufferSize;

            device->CreateConstantBufferView(&constantBufferViewDesc, CBVDescriptorHandle);
        }
    }
}

void demoApp::createShaderResourceView() {
    // 最终创建SRV描述符
    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};

    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format = textureFormat;
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    uint32_t offset = frameResourcesCount * objectCount + 1;

    CD3DX12_CPU_DESCRIPTOR_HANDLE CBVDescriptorHeapHanle(CBVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), offset, CBVSRVUAVDescriptorSize);

    device->CreateShaderResourceView(texture.Get(), &shaderResourceViewDesc, CBVDescriptorHeapHanle);
}

void demoApp::createRootSignature() {
    // 着色器程序一般需要以资源作为输入(例如常量缓冲区、纹理、采样器等)
    // 根签名则定义了着色器程序所需的具体资源，如果把着色器看作一个函数，
    // 而将输入的资源当作向函数传递的参数数据，那么便可类似第认为根签名
    // 定义的是函数签名

    // 根参数可以是描述符表、根描述符或根常量
    // 这里定义了三个根签名参数，对应于Shader代码就是：
    // cbuffer cbPerObject : register(b0)
    // Texture2D texture1 : register(t0, space0)
    // Texture2D texture2 : register(t1, space0)
    // Texture2D texture3 : register(t1, space0)
    CD3DX12_ROOT_PARAMETER1 slotRootParameter[4];

    // 创建由单个CBV所组成的描述符表
    // D3D12_DESCRIPTOR_RANGE1可以看作对应Shader代码中的数组：
    // 
    // In Pixel Shader:
    // Texture2DArray<float4> Tex[10];
    // 
    // In C++ code
    // CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
    //
    // 定义大小为 10 个 SRV 数组
    // ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    CD3DX12_DESCRIPTOR_RANGE1 CBVDescriptorTable[3];
    // 这里的baseShaderRegister如果和Shader代码中的对应不上，就会报错
    // 比如这里填的baseSahderRegister如果设为1，而实际上Shader中的为：

    // In Vertex Shader
    // cbuffer cbPerObject : register(b0)
    // {
    //      float4x4 gWorldViewProj; 
    // };
    //
    // 报错：
    // D3D12 ERROR: ID3D12Device::CreateGraphicsPipelineState: Root Signature doesn't match Vertex Shader: 
    // Shader CBV descriptor range (BaseShaderRegister=0, NumDescriptors=1, RegisterSpace=0) is not fully 
    // bound in root signature [ STATE_CREATION ERROR #688: CREATEGRAPHICSPIPELINESTATE_VS_ROOT_SIGNATURE_MISMATCH]
    CBVDescriptorTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     // rangeType
                                                             1,     // numDescriptors
                                                             0,     // baseShaderRegister
                                                             0,     // registerSpace
                      D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CBVDescriptorTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // rangeType
                                                             1,     // numDescriptors
                                                             0,     // baseShaderRegister
                                                             0,     // registerSpace
                      D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CBVDescriptorTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,      // rangeType
                                                             1,      // numDescriptors
                                                             1,      // baseShaderRegister
                                                             0,      // registerSpace
                      D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    slotRootParameter[0].InitAsDescriptorTable(1, 
                                               &CBVDescriptorTable[0],
                                               D3D12_SHADER_VISIBILITY_VERTEX);

    slotRootParameter[1].InitAsDescriptorTable(1, 
                                               &CBVDescriptorTable[1],
                                               D3D12_SHADER_VISIBILITY_PIXEL);

    slotRootParameter[2].InitAsDescriptorTable(1,                                 // numDescriptorRanges
                                               &CBVDescriptorTable[2],            // pDescriptorRanges
                                               D3D12_SHADER_VISIBILITY_PIXEL      // visibility, visibility to all stages allows sharing binding tables
                                               );

    // D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
    // 表示描述符和纹理数据都是静态的, 设置后不会被修改, 能够获得驱动优化
    CD3DX12_DESCRIPTOR_RANGE1 SRVDescriptorTable;
    SRVDescriptorTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
    slotRootParameter[3].InitAsDescriptorTable(1, &SRVDescriptorTable, D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_STATIC_SAMPLER_DESC samplerDesc;

    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplerDesc.MipLODBias = 0;
    samplerDesc.MaxAnisotropy = 0;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 根签名就是一个根参数构成的段组
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(slotRootParameter), slotRootParameter, 1, &samplerDesc, 
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // 用单个寄存器槽来创建一个根签名，该槽位
    // 指向一个仅含有单个常量缓冲区的描述符区域
    ComPtr<ID3DBlob> serializedRootSignature = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, 
                 &serializedRootSignature, &errorBlob);

    if (errorBlob != nullptr) {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }

    ThrowIfFailed(hr);

    ThrowIfFailed(device->CreateRootSignature(
    0,
    serializedRootSignature->GetBufferPointer(),
    serializedRootSignature->GetBufferSize(),
    IID_PPV_ARGS(rootSignature.GetAddressOf())));
}

void demoApp::createShadersAndInputLayout() {

    vertexShaderByteCode = d3dUtil::compileShader(L"Shaders/color.hlsl", L"VS", L"vs_6_0");
    pixelShaderByteCode = d3dUtil::compileShader(L"Shaders/color.hlsl", L"PS", L"ps_6_0");
    
    inputLayout = 
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
}

void demoApp::createBoxGeometry() {
    std::array<Vertex, 24> vertices = 
    {
        // Front face
        Vertex({XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 0.0f)}),
        Vertex({XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::Black), XMFLOAT2(1.0f, 1.0f)}),
        Vertex({XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::Red),   XMFLOAT2(0.0f, 1.0f)}),
        Vertex({XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::Green), XMFLOAT2(1.0f, 0.0f)}),

        // Right side face
        Vertex({XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::Blue),    XMFLOAT2(0.0f, 1.0f)}),
        Vertex({XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::Yellow),  XMFLOAT2(1.0f, 0.0f)}),
        Vertex({XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::Cyan),    XMFLOAT2(1.0f, 1.0f)}),
        Vertex({XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::Magenta), XMFLOAT2(0.0f, 0.0f)}),

        // Left side face
        Vertex({XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 0.0f)}),
        Vertex({XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 1.0f)}),
        Vertex({XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 1.0f)}),
        Vertex({XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 0.0f)}),

        // Back face
        Vertex({XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 0.0f)}),
        Vertex({XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 1.0f)}),
        Vertex({XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 1.0f)}),
        Vertex({XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 0.0f)}),

        // Top face
        Vertex({XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 1.0f)}),
        Vertex({XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 0.0f)}),
        Vertex({XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 1.0f)}),
        Vertex({XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 0.0f)}),

        // Bttom faceX
        Vertex({XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 0.0f)}),
        Vertex({XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 1.0f)}),
        Vertex({XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 1.0f)}),
        Vertex({XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 0.0f)})
    };

    std::array<std::uint16_t, 36> indices = 
    {
        // Front face
        0, 1, 2,
        0, 3, 1,

        // Left face
        4, 5, 6,
        4, 7, 5,

        // Right face
        8, 9, 10, //
        8, 11, 9, //

        // Back face
        12, 13, 14, 
        12, 15, 13, 

        // Top face
        16, 17, 18, 
        16, 19, 17, 

        // Bottom face
        20, 21, 22, 
        20, 23, 21, 
    };

    const UINT vertexBufferByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT indexBufferByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    boxGeometry = std::make_unique<MeshGeometry>();
    boxGeometry->Name = "BoxGeometry";

    ThrowIfFailed(D3DCreateBlob(vertexBufferByteSize, &boxGeometry->VertexBufferCPU));
    CopyMemory(boxGeometry->VertexBufferCPU->GetBufferPointer(),
    vertices.data(), vertexBufferByteSize);

    ThrowIfFailed(D3DCreateBlob(indexBufferByteSize, &boxGeometry->IndexBufferCPU));
    CopyMemory(boxGeometry->IndexBufferCPU->GetBufferPointer(),
    indices.data(), indexBufferByteSize);

    // 重置命令列表为执行初始化命令做好准备工作
    ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

    boxGeometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(), commandList.Get(),
    vertices.data(), vertexBufferByteSize, boxGeometry->VertexBufferUploader);

    boxGeometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(), commandList.Get(),
    indices.data(), indexBufferByteSize, boxGeometry->IndexBufferUploader);

    // 执行初始化命令
    ThrowIfFailed(commandList->Close());
    ID3D12CommandList* commandLists[] = {commandList.Get()};
    commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    boxGeometry->VertexByteStride = sizeof(Vertex);
    boxGeometry->VertexBufferByteSize = vertexBufferByteSize;
    boxGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
    boxGeometry->IndexBufferByteSize = indexBufferByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    boxGeometry->DrawArgs["Box"] = submesh;
}

void demoApp::createPipelineStateOjbect() {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateObjectDesc;

    ZeroMemory(&pipelineStateObjectDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    pipelineStateObjectDesc.InputLayout = {inputLayout.data(), (UINT)inputLayout.size()};
    pipelineStateObjectDesc.pRootSignature = rootSignature.Get();

    std::string vertexShader;
    
    d3dUtil::loadShader("Shaders/vs.bin", vertexShader);

    pipelineStateObjectDesc.VS.pShaderBytecode = vertexShader.data();
    pipelineStateObjectDesc.VS.BytecodeLength = vertexShader.size();

    std::string pixelShader;

     d3dUtil::loadShader("Shaders/ps.bin", pixelShader);

    pipelineStateObjectDesc.PS.pShaderBytecode = pixelShader.data();
    pipelineStateObjectDesc.PS.BytecodeLength = pixelShader.size();

    // pipelineStateObjectDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderByteCode.Get());
    // pipelineStateObjectDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderByteCode.Get());

    D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    // rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;

    pipelineStateObjectDesc.RasterizerState = rasterizerDesc;
    pipelineStateObjectDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pipelineStateObjectDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pipelineStateObjectDesc.SampleMask = UINT_MAX;
    pipelineStateObjectDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateObjectDesc.NumRenderTargets = 1;
    pipelineStateObjectDesc.RTVFormats[0] = backBufferFormat;
    pipelineStateObjectDesc.SampleDesc.Count = 1;
    pipelineStateObjectDesc.SampleDesc.Quality = 0;
    pipelineStateObjectDesc.DSVFormat = depthStencilBufferFormat;
    
    ThrowIfFailed(device->CreateGraphicsPipelineState(&pipelineStateObjectDesc, IID_PPV_ARGS(pipelineStateObject.GetAddressOf())));
}

void demoApp::loadResources() {
    uint32_t textureWidth = 0;
    uint32_t textureHeight = 0;
    uint32_t bpp = 0;
    uint32_t rowPitch = 0;

    ComPtr<IWICBitmapSource> image = WICLoadImage(L"Textures/Kanna.gif", textureWidth, textureHeight, bpp, rowPitch, textureFormat);

    D3D12_RESOURCE_DESC textureDesc = {};

    // Alignment must be 64KB (D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) or 0, which is effectively 64KB.
    // textureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.MipLevels = 1;
    textureDesc.Format = textureFormat;
    textureDesc.Width = textureWidth;
    textureDesc.Height = textureHeight;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;

    // 创建默认堆上的资源,类型是Texture2D,GPU对默认堆资源的访问速度是最快的
    // 因为纹理资源一般是不易变的资源,所以我们通常使用上传堆复制到默认堆中
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,                    // 可以使用CD3DX12_RESOURCE_DESC::Tex2D来简化结构体的初始化
        D3D12_RESOURCE_STATE_COPY_DEST,  // 资源状态(权限标志)
        nullptr,
        IID_PPV_ARGS(texture.GetAddressOf())));

    // 获取上传堆资源缓冲的大小,这个尺寸通常大于实际图片的尺寸
    const uint64_t uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);

    // 创建用于上传纹理的资源，注意其类型是Buffer。上传堆对于GPU访问来说性能是很差的
    // 所以对于几乎不变的数据尤其像纹理都是通过它来上传至GPU访问更高效的默认堆中
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(textureUpload.GetAddressOf())));

    // 按照资源缓冲大小来分配实际图片数据存储的内存大小
    void* imageData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, uploadBufferSize);

    if (imageData == nullptr) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    // 从图片中取出数据
    ThrowIfFailed(image->CopyPixels(
        nullptr,
        rowPitch,
        static_cast<uint32_t>(rowPitch * textureHeight),    // 注意这里才是图片数据真实的大小，这个值通常小于缓冲的大小
        reinterpret_cast<byte*>(imageData)));

    // 获取向上传堆拷贝纹理数据的一些纹理转换尺寸信息
    // 对于复杂的DDS纹理这是非常必要的过程
    uint64_t requiredSize = 0;
    uint32_t numSubresources = 1;   // 我们只有一副图片，即子资源个数为1
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureLayouts;
    uint64_t textureRowSizes = 0;
    uint32_t textureRowNum = 0;

    D3D12_RESOURCE_DESC resourceDesc = texture->GetDesc();

    device->GetCopyableFootprints(
        &resourceDesc,
        0,
        numSubresources,
        0,
        &textureLayouts,
        &textureRowNum,
        &textureRowSizes,
        &requiredSize);

    // 因为上传堆实际就是CPU传递数据到GPU的中介，所以我们可以使用
    // 熟悉的Map方法将它先映射到CPU内存地址中，然后我们按行将数据复制
    // 到上传堆中。需要注意的是之所以按行拷贝是因为GPU资源的行大小
    // 与实际图片的行大小是有差异的，二者的内存边界对齐要求是不一样的
    byte* data = nullptr;

    ThrowIfFailed(textureUpload->Map(0, nullptr, reinterpret_cast<void**>(&data)));

    byte* destSlice = reinterpret_cast<byte*>(data) + textureLayouts.Offset;

    const byte* sourceSlice = reinterpret_cast<const byte*>(imageData);

    for (uint32_t row = 0; row < textureRowNum; row++) {
        memcpy_s(destSlice + static_cast<SIZE_T>(textureLayouts.Footprint.RowPitch) * row, rowPitch,
                 sourceSlice + static_cast<SIZE_T>(rowPitch) * row, rowPitch);
    }

    // 取消映射，对于易变的数据如每帧的变换矩阵等数据，可以先不Unmap
    // 让它常驻内存，以提高整体性能，因为每次Map和Unmap是非常耗时的操作
    textureUpload->Unmap(0, nullptr);

    // 释放图片数据，做一个干净的程序员
    ::HeapFree(::GetProcessHeap(), 0, imageData);

    // 重置命令列表为执行初始化命令做好准备工作
    ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

    // 向命令队列发出从上传堆复制纹理数据到默认堆的命令
    CD3DX12_TEXTURE_COPY_LOCATION dest(texture.Get(), 0);
    CD3DX12_TEXTURE_COPY_LOCATION source(textureUpload.Get(), textureLayouts);
    commandList->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);

    // 设置一个资源屏障，同步并确认复制操作完成
    D3D12_RESOURCE_BARRIER resourceBarrier;
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resourceBarrier.Transition.pResource = texture.Get();
    resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList->ResourceBarrier(1, &resourceBarrier);

    ThrowIfFailed(commandList->Close());
    ID3D12CommandList* commandLists[] = {commandList.Get()};
    commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void demoApp::prepareFrameResourceSync() {
    currentFrameResource->fenceValue = ++currentFenceValue;
    commandQueue->Signal(fence.Get(), currentFenceValue);
}

void demoApp::frameResourceSync() {
    //每帧遍历一个帧资源（多帧的话就是环形遍历）
    currentFrameIndex = (currentFrameIndex + 1) % frameResourcesCount;
    currentFrameResource = frameResources[currentFrameIndex].get();

    // GPU是否已经处理完当前帧资源的命令？
    // 如果没有则等待直到GPU完成
    uint64_t currentFrameFenceValue = currentFrameResource->fenceValue;

    if (currentFrameFenceValue != 0 && fence->GetCompletedValue() < currentFrameFenceValue) {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(fence->SetEventOnCompletion(currentFrameFenceValue, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void demoApp::initImGUI() 
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(mainWindow);
	ImGui_ImplDX12_Init(device.Get(), frameResourcesCount, 
		DXGI_FORMAT_R8G8B8A8_UNORM,
		SRVDescriptorHeap.Get(),
		SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void demoApp::buildImGuiWidgets()
{
	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (showDemoWindow)
		ImGui::ShowDemoWindow(&showDemoWindow);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		if (showMainWindow)
		{
			if (!ImGui::Begin("Hello, world!", &showMainWindow))             // Create a window called "Hello, world!" and append into it.
			{
				// Early out if the window is collapsed, as an optimization.
				ImGui::End();
				return;
			}

			ImGui::Text("This is some useful text.");             // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &showDemoWindow);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &showAnotherWindow);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clearColor); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}
	}

	// 3. Show another simple window.
	if (showAnotherWindow)
	{
		ImGui::Begin("Another Window", &showAnotherWindow);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			showAnotherWindow = false;
		ImGui::End();
	}
}

void demoApp::updateImGui()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void demoApp::renderImGui()
{
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    // 创建控制台程序
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONIN$", "r+t", stdin);
    freopen_s(&stream, "CONOUT$", "w+t", stdout);

	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    try
    {
        demoApp app(hInstance, 1280, 720);
        if(!app.initialize())
            return 0;

        return app.run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }

    // 程序退出之前关闭标准输入输出
    fclose(stdin);
    fclose(stdout);
}