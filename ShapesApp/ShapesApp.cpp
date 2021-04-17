﻿#include "ShapesApp.h"
#include "Common/d3dUtil.h"
#include "Common/WICUtils.h"
#include "Common/GeometryGenerator.h"
#include <DirectXColors.h>

#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

#include <iostream>

shapesApp::shapesApp(HINSTANCE inInstance, const uint32_t inWindowWidth, const uint32_t inWindowHeight)
: d3dApp(inInstance, inWindowWidth, inWindowHeight) {

}

shapesApp::~shapesApp() {
    frameResourceSync();

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT shapesApp::msgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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

bool shapesApp::initialize() {
    if(!d3dApp::initialize()) {
        return false;
    }

    resetCommandList();

    createBoxGeometry();
    buildShapeGeometry();
    buildRenderItems();
    loadResources();

    executeCommandList();

    createCBVSRVDescriptorHeaps();
    createFrameResources();
    createConstantBufferViews();
    createShaderResourceView();
    createSampler();
    createRootSignature();
    createShadersAndInputLayout();
    createPipelineStateOjbect();

    initImGUI();

	return true;
}

void shapesApp::onResize() {
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

void shapesApp::update(float delta) {
    frameResourceSync();

    updateObjectConstantBuffers();
    updatePassConstantBuffers(); 

    updateImGui();
	buildImGuiWidgets();
}

void shapesApp::draw(float delta) {
    // 重复使用记录命令的相关内存
    // 只有当与GPU关联的命令列表执行完成时，我们才能将其重置
    commandAllocator = currentFrameResource->commandAllocator;
    ThrowIfFailed(commandAllocator->Reset());

    // 在通过ExecuteCommandList方法将某个命令列表加入命令队列后，
    // 我们便可以重置该命令列表。以此来复用命令列表及其内存
    if (isWireframe) {
        ThrowIfFailed(commandList->Reset(
            commandAllocator.Get(), graphicsPSOs["opaque_wireframe"].Get()));
    }
    else {
        ThrowIfFailed(commandList->Reset(commandAllocator.Get(), graphicsPSOs["opaque"].Get()));
    }

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

    ID3D12DescriptorHeap* descriptorHeaps[] = {CBVDescriptorHeap.Get(), samplerDescriptorHeap.Get()};

    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootSignature(rootSignature.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &boxGeometry->VertexBufferView());
    commandList->IASetIndexBuffer(&boxGeometry->IndexBufferView());

    uint32_t passConstantBufferIndex = frameResourcesCount * objectCount + currentFrameIndex;

    drawRenderItems(opaqueRenderItems);
   
    uint32_t SRVDescriptorIndex = (objectCount + 1) * frameResourcesCount;

    CD3DX12_GPU_DESCRIPTOR_HANDLE SRVDescriptorHandle(CBVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), SRVDescriptorIndex, CBVSRVUAVDescriptorSize);

    commandList->SetDescriptorHeaps(1, SRVDescriptorHeap.GetAddressOf());
    commandList->SetGraphicsRootDescriptorTable(4, SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

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
void shapesApp::onMouseDown(WPARAM btnState, int32_t x, int32_t y) {
    lastMousePosition.x = x;
    lastMousePosition.y = y;

    SetCapture(mainWindow);
}

void shapesApp::onMouseUp(WPARAM btnState, int32_t x, int32_t y) {
    ReleaseCapture();
}

void shapesApp::onMouseMove(WPARAM btnState, int32_t x, int32_t y) {
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

void shapesApp::onKeyDown(WPARAM btnState) {
    if (btnState == VK_F2) {
        isWireframe = !isWireframe;
    }
}

void shapesApp::onKeyUp(WPARAM btnState) {

}

void shapesApp::createCBVSRVDescriptorHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC CBVDescriptorHeapDesc;
    // objectCount * frameBackBufferCount + CRV(1) + SRV(1)
    objectCount = static_cast<uint32_t>(allRenderItems.size());
    CBVDescriptorHeapDesc.NumDescriptors = (objectCount + 1 + 1) * frameBackBufferCount + static_cast<uint32_t>(textures.size());
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

    D3D12_DESCRIPTOR_HEAP_DESC samplerDescriptorHeapDesc;
    samplerDescriptorHeapDesc.NumDescriptors = 1;
    samplerDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    samplerDescriptorHeapDesc.NodeMask = 0;
    ThrowIfFailed(device->CreateDescriptorHeap(&samplerDescriptorHeapDesc, IID_PPV_ARGS(samplerDescriptorHeap.GetAddressOf())));
}

void shapesApp::createFrameResources() {
    for (int frameResourceIndex = 0; frameResourceIndex < frameBackBufferCount; frameResourceIndex++) {
        frameResources.push_back(std::make_unique<FrameUtil::FrameResources>(device.Get(), objectCount, 1, 1));
    }
}

void shapesApp::createConstantBufferViews() {

    UINT objectConstantBufferSize = d3dUtil::CalcConstantBufferByteSize(sizeof(FrameUtil::ObjectConstants));

    for (size_t frameResourcesIndex = 0; frameResourcesIndex < frameResourcesCount; frameResourcesIndex++) {
        
        for (size_t objectIndex = 0; objectIndex < objectCount; objectIndex++) {
            auto& objectConstantBuffer = frameResources[frameResourcesIndex]->objectConstantBuffer;
            
            D3D12_GPU_VIRTUAL_ADDRESS objectConstantBufferAddress = objectConstantBuffer->Resource()->GetGPUVirtualAddress();

            // 偏移到常量缓冲区中第i个物体所对应的常量数据
            objectConstantBufferAddress += objectIndex * objectConstantBufferSize;

            CD3DX12_CPU_DESCRIPTOR_HANDLE CBVDescriptorHandle(CBVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

            int32_t descriptorHeapIndex = static_cast<uint32_t>(frameResourcesIndex) * objectCount + static_cast<uint32_t>(objectIndex);

            CBVDescriptorHandle.Offset(descriptorHeapIndex, CBVSRVUAVDescriptorSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc;
            constantBufferViewDesc.BufferLocation = objectConstantBufferAddress;
            constantBufferViewDesc.SizeInBytes = objectConstantBufferSize;

            device->CreateConstantBufferView(&constantBufferViewDesc, CBVDescriptorHandle);
        }
    }

    uint32_t passConstantBufferSize = d3dUtil::CalcConstantBufferByteSize(sizeof(FrameUtil::PassConstants));

    for (uint32_t frameResourcesIndex = 0; frameResourcesIndex < frameResourcesCount; frameResourcesIndex++) {
        auto& passConstantBuffer = frameResources[frameResourcesIndex]->passConstantBuffer;

        D3D12_GPU_VIRTUAL_ADDRESS passConstantBufferAddress = passConstantBuffer->Resource()->GetGPUVirtualAddress();

        CD3DX12_CPU_DESCRIPTOR_HANDLE CBVDescriptorHandle(CBVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        uint32_t descriptorHeapIndex = frameResourcesCount * objectCount + frameResourcesIndex;

        CBVDescriptorHandle.Offset(descriptorHeapIndex, CBVSRVUAVDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc;
        constantBufferViewDesc.BufferLocation = passConstantBufferAddress;
        constantBufferViewDesc.SizeInBytes = passConstantBufferSize;

        device->CreateConstantBufferView(&constantBufferViewDesc, CBVDescriptorHandle);
    }

    uint32_t materialConstantBufferSize = d3dUtil::CalcConstantBufferByteSize(sizeof(FrameUtil::MaterialConstants));

    for (uint32_t frameResourceIndex = 0; frameResourceIndex < frameResourcesCount; frameResourceIndex++) {
        auto& materialConstantBuffer = frameResources[frameResourceIndex]->materialConstantBuffer;

        D3D12_GPU_VIRTUAL_ADDRESS materialConstantBufferAddress = materialConstantBuffer->Resource()->GetGPUVirtualAddress();

        CD3DX12_CPU_DESCRIPTOR_HANDLE CBVDescriptorHandle(CBVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        uint32_t descriptorHeapIndex = frameResourcesCount * (objectCount + 1) + frameResourceIndex;

        CBVDescriptorHandle.Offset(descriptorHeapIndex, CBVSRVUAVDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc;
        constantBufferViewDesc.BufferLocation = materialConstantBufferAddress;
        constantBufferViewDesc.SizeInBytes = materialConstantBufferSize;

        device->CreateConstantBufferView(&constantBufferViewDesc, CBVDescriptorHandle);
    }
}

void shapesApp::createShaderResourceView() {
    // 最终创建SRV描述符
    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};

    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format = textureFormat;
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    uint32_t descriptorHeapIndex = (objectCount + 2) * frameBackBufferCount;

    CD3DX12_CPU_DESCRIPTOR_HANDLE CBVDescriptorHeapHanle(CBVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    CBVDescriptorHeapHanle.Offset(descriptorHeapIndex, CBVSRVUAVDescriptorSize);

    for (size_t textureIndex = 0; textureIndex < textures.size(); textureIndex++) {
        device->CreateShaderResourceView(textures[textureIndex].Get(), &shaderResourceViewDesc, CBVDescriptorHeapHanle);

        CBVDescriptorHeapHanle.Offset(CBVSRVUAVDescriptorSize);
    }
}

void shapesApp::createSampler() {
    D3D12_SAMPLER_DESC samplerDesc;

    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplerDesc.MipLODBias = 0;
    samplerDesc.MaxAnisotropy = 0;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplerDesc.BorderColor[0] = 0.0f;
    samplerDesc.BorderColor[1] = 0.0f;
    samplerDesc.BorderColor[2] = 0.0f;
    samplerDesc.BorderColor[3] = 0.0f;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    device->CreateSampler(&samplerDesc, samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void shapesApp::createRootSignature() {
    // 着色器程序一般需要以资源作为输入(例如常量缓冲区、纹理、采样器等)
    // 根签名则定义了着色器程序所需的具体资源，如果把着色器看作一个函数，
    // 而将输入的资源当作向函数传递的参数数据，那么便可类似第认为根签名
    // 定义的是函数签名

    // 根参数可以是描述符表、根描述符或根常量
    // 这里定义了三个根签名参数，对应于Shader代码就是：
    // cbuffer constantBufferPerObject : register(b0, space0)
    // cbuffer constantBufferPerPass : register(b1, space0)
    // Texture2D texture1 : register(t0, space0)
    // Texture2D texture2 : register(t1, space0)
    CD3DX12_ROOT_PARAMETER1 slotRootParameter[6];

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
    CD3DX12_DESCRIPTOR_RANGE1 CBVDescriptorTable[4];
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

    CBVDescriptorTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     // rangeType
                                                             1,     // numDescriptors
                                                             1,     // baseShaderRegister
                                                             0,     // registerSpace
                      D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CBVDescriptorTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     // rangeType
                                                             1,     // numDescriptors
                                                             2,     // baseShaderRegister
                                                             0,     // registerSpace
                      D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CBVDescriptorTable[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,      // rangeType
                        static_cast<uint32_t>(textures.size()),      // numDescriptors
                                                             0,      // baseShaderRegister
                                                             0,      // registerSpace
                      D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    slotRootParameter[0].InitAsDescriptorTable(1, 
                                               &CBVDescriptorTable[0],
                                               D3D12_SHADER_VISIBILITY_VERTEX);

    slotRootParameter[1].InitAsDescriptorTable(1, 
                                               &CBVDescriptorTable[1],
                                               D3D12_SHADER_VISIBILITY_VERTEX);

    slotRootParameter[2].InitAsDescriptorTable(1, 
                                               &CBVDescriptorTable[2],
                                               D3D12_SHADER_VISIBILITY_PIXEL);

    slotRootParameter[3].InitAsDescriptorTable(1,                                       // numDescriptorRanges
                                               &CBVDescriptorTable[3],                  // pDescriptorRanges
                                               D3D12_SHADER_VISIBILITY_PIXEL            // visibility, visibility to all stages allows sharing binding tables
                                               );

    // D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
    // 表示描述符和纹理数据都是静态的, 设置后不会被修改, 能够获得驱动优化
    CD3DX12_DESCRIPTOR_RANGE1 SRVDescriptorTable;
    SRVDescriptorTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, static_cast<uint32_t>(textures.size()), 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
    slotRootParameter[4].InitAsDescriptorTable(1, &SRVDescriptorTable, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_DESCRIPTOR_RANGE1 samplerDescriptorTable;
    samplerDescriptorTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0);
    slotRootParameter[5].InitAsDescriptorTable(1, &samplerDescriptorTable, D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_STATIC_SAMPLER_DESC samplerDesc;

    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplerDesc.MipLODBias = 0;
    samplerDesc.MaxAnisotropy = 0;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 根签名就是一个根参数构成的段组
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    // rootSignatureDesc.Init_1_1(_countof(slotRootParameter), slotRootParameter, 1, &samplerDesc, 
    // D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    rootSignatureDesc.Init_1_1(_countof(slotRootParameter), slotRootParameter, 0, nullptr, 
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

void shapesApp::createShadersAndInputLayout() {

    vertexShaderByteCode = d3dUtil::compileShader(L"Shaders/color.hlsl", L"VS", L"vs_6_0");
    pixelShaderByteCode = d3dUtil::compileShader(L"Shaders/color.hlsl", L"PS", L"ps_6_0");
    
    inputLayout = 
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
}

void shapesApp::createBoxGeometry() {
    // std::array<Vertex, 24> vertices = 
    // {
    //     // Front face
    //     Vertex({XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 0.0f)}),
    //     Vertex({XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::Black), XMFLOAT2(1.0f, 1.0f)}),
    //     Vertex({XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::Red),   XMFLOAT2(0.0f, 1.0f)}),
    //     Vertex({XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::Green), XMFLOAT2(1.0f, 0.0f)}),

    //     // Right side face
    //     Vertex({XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::Blue),    XMFLOAT2(0.0f, 1.0f)}),
    //     Vertex({XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::Yellow),  XMFLOAT2(1.0f, 0.0f)}),
    //     Vertex({XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::Cyan),    XMFLOAT2(1.0f, 1.0f)}),
    //     Vertex({XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::Magenta), XMFLOAT2(0.0f, 0.0f)}),

    //     // Left side face
    //     Vertex({XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 0.0f)}),
    //     Vertex({XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 1.0f)}),
    //     Vertex({XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 1.0f)}),
    //     Vertex({XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 0.0f)}),

    //     // Back face
    //     Vertex({XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 0.0f)}),
    //     Vertex({XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 1.0f)}),
    //     Vertex({XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 1.0f)}),
    //     Vertex({XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 0.0f)}),

    //     // Top face
    //     Vertex({XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 1.0f)}),
    //     Vertex({XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 0.0f)}),
    //     Vertex({XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 1.0f)}),
    //     Vertex({XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 0.0f)}),

    //     // Bttom faceX
    //     Vertex({XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 0.0f)}),
    //     Vertex({XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 1.0f)}),
    //     Vertex({XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::White), XMFLOAT2(0.0f, 1.0f)}),
    //     Vertex({XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(Colors::White), XMFLOAT2(1.0f, 0.0f)})
    // };

    // std::array<std::uint16_t, 36> indices = 
    // {
    //     // Front face
    //     0, 1, 2,
    //     0, 3, 1,

    //     // Left face
    //     4, 5, 6,
    //     4, 7, 5,

    //     // Right face
    //     8, 9, 10, //
    //     8, 11, 9, //

    //     // Back face
    //     12, 13, 14, 
    //     12, 15, 13, 

    //     // Top face
    //     16, 17, 18, 
    //     16, 19, 17, 

    //     // Bottom face
    //     20, 21, 22, 
    //     20, 23, 21, 
    // };
    GeometryGenerator geometryGenerator;
    GeometryGenerator::MeshData box = geometryGenerator.CreateBox(1.0f, 1.0f, 1.0f, 0);

    const UINT vertexBufferByteSize = (UINT)box.Vertices.size() * sizeof(FrameUtil::Vertex);
    const UINT indexBufferByteSize = (UINT)box.Indices32.size() * sizeof(std::uint32_t);

    boxGeometry = std::make_unique<MeshGeometry>();
    boxGeometry->Name = "BoxGeometry";

    ThrowIfFailed(D3DCreateBlob(vertexBufferByteSize, &boxGeometry->VertexBufferCPU));
    CopyMemory(boxGeometry->VertexBufferCPU->GetBufferPointer(),
    box.Vertices.data(), vertexBufferByteSize);

    ThrowIfFailed(D3DCreateBlob(indexBufferByteSize, &boxGeometry->IndexBufferCPU));
    CopyMemory(boxGeometry->IndexBufferCPU->GetBufferPointer(),
    box.Indices32.data(), indexBufferByteSize);

    std::vector<FrameUtil::Vertex> vertices(box.Vertices.size());

    for (size_t i = 0; i < box.Vertices.size(); i++) {
        vertices[i].position = box.Vertices[i].Position;
        vertices[i].uv = box.Vertices[i].TexC;
    }

    boxGeometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(), commandList.Get(),
    vertices.data(), vertexBufferByteSize, boxGeometry->VertexBufferUploader);

    boxGeometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(), commandList.Get(),
    box.Indices32.data(), indexBufferByteSize, boxGeometry->IndexBufferUploader);

    boxGeometry->VertexByteStride = sizeof(FrameUtil::Vertex);
    boxGeometry->VertexBufferByteSize = vertexBufferByteSize;
    boxGeometry->IndexFormat = DXGI_FORMAT_R32_UINT;
    boxGeometry->IndexBufferByteSize = indexBufferByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)box.Indices32.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    boxGeometry->DrawArgs["Box"] = submesh;
}

void shapesApp::createPipelineStateOjbect() {
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
    
    // ComPtr的行为类似于std::shared_ptr，要注意不要不小心覆盖了指针导致内存泄漏
    // 比如如下代码中，如果不声明两个ComPtr<ID3D12PipelineState，只使用一个的话，
    // 后面创建的ID3D12PipelineState就会使前面创建的ID3D12PipelineState变成野指针
    // ComPtr析构的时候只会删除后面创建的ID3D12PipelineState，从而导致内存泄露
    ComPtr<ID3D12PipelineState> opaquePipelineStateObject = nullptr;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&pipelineStateObjectDesc, IID_PPV_ARGS(opaquePipelineStateObject.GetAddressOf())));

    graphicsPSOs["opaque"] = opaquePipelineStateObject;

    pipelineStateObjectDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

    ComPtr<ID3D12PipelineState> opaqueWireframePipelineStateObject = nullptr;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&pipelineStateObjectDesc, IID_PPV_ARGS(opaqueWireframePipelineStateObject.GetAddressOf())));

    graphicsPSOs["opaque_wireframe"] = opaqueWireframePipelineStateObject;
}

void shapesApp::loadResources() {
    uint32_t textureWidth = 0;
    uint32_t textureHeight = 0;
    uint32_t bpp = 0;
    uint32_t rowPitch = 0;

    // auto images = WICLoadImage(L"Textures/Kanna.gif", textureWidth, textureHeight, bpp, rowPitch, textureFormat);
    auto images = WICLoadImage(L"Textures/Kanna0.gif", textureWidth, textureHeight, bpp, rowPitch, textureFormat);

    D3D12_RESOURCE_DESC textureDesc = {};

    // Alignment must be 64KB (D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) or 0, which is effectively 64KB.
    textureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.MipLevels = 1;
    textureDesc.Format = textureFormat;
    textureDesc.Width = textureWidth;
    textureDesc.Height = textureHeight;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;

    textures.resize(images.size());

    for (size_t imageIndex = 0; imageIndex < images.size(); imageIndex++) {
        // 创建默认堆上的资源,类型是Texture2D,GPU对默认堆资源的访问速度是最快的
        // 因为纹理资源一般是不易变的资源,所以我们通常使用上传堆复制到默认堆中
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,                    // 可以使用CD3DX12_RESOURCE_DESC::Tex2D来简化结构体的初始化
            D3D12_RESOURCE_STATE_COPY_DEST,  // 资源状态(权限标志)
            nullptr,
            IID_PPV_ARGS(textures[imageIndex].GetAddressOf())));   
    }

    // 获取上传堆资源缓冲的大小,这个尺寸通常大于实际图片的尺寸
    // 注意：这里要将上传堆的尺寸大小对齐到D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT，
    // 因为后面调用UpdateSubresources的时候，里面的CopyTextureRegion期望传入的
    // Offset是512的倍数，否则就会报以下的错误：
    // D3D12 ERROR: ID3D12CommandList::CopyTextureRegion: 
    // D3D12_PLACED_SUBRESOURCE_FOOTPRINT::Offset must be a multiple of 512, 
    // aka. D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT. Offset is 1023952.
    // [ RESOURCE_MANIPULATION ERROR #864: COPYTEXTUREREGION_INVALIDSRCOFFSET]
    // 官方示例中直接用的GetRequiredIntermediateSize获取到的地址不会有问题
    // 因为它使用的纹理尺寸是2的N次方，通过GetRequiredIntermediateSize获取
    // 到的尺寸就是满足要求的。搞了一晚上终于搞定了纹理数组的创建和使用，泪奔~~
    // 纹理尺寸是32 x 32，GetRequiredIntermediateSize返回8064(比纹理大小32 x 32 x 4(4096)多了几乎一倍)
    // 纹理尺寸是64 x 64，GetRequiredIntermediateSize返回10384(和纹理大小一样, 64 x 64 x 4)
    // 纹理尺寸是128 x 128，GetRequiredIntermediateSize返回65536(和纹理大小一样, 128 x 128 x 4)
    //              Default	Small
    // Buffer	    64 KB	 
    // Texture	    64 KB	4 KB
    // MSAA texture	4 MB	64 KB
    const uint64_t uploadBufferStep = d3dUtil::Align(GetRequiredIntermediateSize(textures[0].Get(), 0, 1), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    // const uint64_t uploadBufferStep = GetRequiredIntermediateSize(textures[0].Get(), 0, 1);
    const uint64_t uploadBufferSize = uploadBufferStep * textures.size();

    device->GetResourceAllocationInfo(0, 1, &textureDesc);

    // 创建用于上传纹理的资源，注意其类型是Buffer。上传堆对于GPU访问来说性能是很差的
    // 所以对于几乎不变的数据尤其像纹理都是通过它来上传至GPU访问更高效的默认堆中
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(textureUpload.GetAddressOf())));

    for (size_t textureIndex = 0; textureIndex < textures.size(); textureIndex++) {
        // 按照资源缓冲大小来分配实际图片数据存储的内存大小
        void* imageData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, uploadBufferStep);

        if (imageData == nullptr) {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        auto image = images[textureIndex];

        // 从图片中取出数据
        HRESULT hr = image->CopyPixels(
            nullptr,
            rowPitch,
            static_cast<uint32_t>(rowPitch * textureHeight),    // 注意这里才是图片数据真实的大小，这个值通常小于缓冲的大小
            reinterpret_cast<byte*>(imageData));

        ThrowIfFailed(hr);

        std::string fileName = "test" + std::to_string(textureIndex) + ".png"; 

        // saveImage(fileName, reinterpret_cast<byte*>(imageData), textureWidth, textureHeight);

        // 获取向上传堆拷贝纹理数据的一些纹理转换尺寸信息
        // 对于复杂的DDS纹理这是非常必要的过程
        uint64_t requiredSize = 0;
        uint32_t numSubresources = 1;   // 我们只有一副图片，即子资源个数为1
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureLayouts;
        uint64_t textureRowSizes = 0;
        uint32_t textureRowNum = 0;

        D3D12_RESOURCE_DESC resourceDesc = textures[textureIndex]->GetDesc();

        // 不要把pRowSizeInBytes和D3D12_SUBRESOURCE_FOOTPRINT的RowPitch搞混，
        // 因为RowPitch是对齐到D3D12_TEXTURE_DATA_PITCH_ALIGNMENT的值
        // 例如，纹理的宽度为32，每像素4个字节，pRowSizeInBytes的值为128
        // 而RowPitch的值则是256
        device->GetCopyableFootprints(
            &resourceDesc,
            0,
            numSubresources,
            uploadBufferStep * textureIndex,
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
        // textureUpload已经经过了对齐操作，所以这里的偏移需要使用D3D12_SUBRESOURCE_FOOTPRINT的RowPitch字段，
        // 因为经过GetCopyableFootprints函数的填充，RowPitch是按D3D12_TEXTURE_DATA_PITCH_ALIGNMENT对齐的
        // 而拷贝的时候就需要图片的原始的rowPitch了，之前就是搞混了两者，导致对UpdateSubresources函数的调用
        // 要么崩溃，要么渲染结果出错
        for (uint32_t row = 0; row < textureRowNum; row++) {
            memcpy_s(destSlice + static_cast<SIZE_T>(textureLayouts.Footprint.RowPitch) * row, rowPitch,
                     sourceSlice + static_cast<SIZE_T>(rowPitch) * row, rowPitch);
        }

        // 取消映射，对于易变的数据如每帧的变换矩阵等数据，可以先不Unmap
        // 让它常驻内存，以提高整体性能，因为每次Map和Unmap是非常耗时的操作
        textureUpload->Unmap(0, nullptr);

        // {
        //     // Copy data to the intermediate upload heap and then schedule
        //     // a copy from the upload heap to the appropriate texture
        //     // const UINT D3D12_TEXTURE_DATA_PITCH_ALIGNMENT = 256;        纹理行按256字节对齐
        //     // const UINT D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT = 512;    纹理整体按512字节
        //     // 照着官方示例抄代码抄出问题来了，示例中纹理的尺寸大小本身就是对齐的
        //     D3D12_SUBRESOURCE_DATA  textureData = {};
        //     textureData.pData = imageData;
        //     uint32_t channels = bpp / 8;
        //     textureData.RowPitch =  rowPitch;
        //     textureData.SlicePitch = rowPitch * textureDesc.Height;

        //     D3D12_RESOURCE_ALLOCATION_INFO info = device->GetResourceAllocationInfo(0, 1, &resourceDesc);

        //     UpdateSubresources(commandList.Get(), textures[textureIndex].Get(), textureUpload.Get(), textureIndex * uploadBufferStep, 0, 1, &textureData);
        // }

        // 释放图片数据，做一个干净的程序员
        ::HeapFree(::GetProcessHeap(), 0, imageData);

        // 向命令队列发出从上传堆复制纹理数据到默认堆的命令
        // textureLayouts中包含了在textureUpload中的偏移
        // (D3D12_PLACED_SUBRESOURCE_FOOTPRINT::Offset字段) 
        CD3DX12_TEXTURE_COPY_LOCATION dest(textures[textureIndex].Get(), 0);
        CD3DX12_TEXTURE_COPY_LOCATION source(textureUpload.Get(), textureLayouts);
        commandList->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);

        // 设置一个资源屏障，同步并确认复制操作完成
        D3D12_RESOURCE_BARRIER resourceBarrier;
        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        resourceBarrier.Transition.pResource = textures[textureIndex].Get();
        resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(1, &resourceBarrier);
    }
}

void shapesApp::buildShapeGeometry() {
    GeometryGenerator geometryGenerator;

    GeometryGenerator::MeshData box = geometryGenerator.CreateBox(1.0f, 1.0f, 1.0f, 0);
    GeometryGenerator::MeshData grid = geometryGenerator.CreateGrid(10.0f, 10.0f, 10, 10);

    // 将所有的结合体数据都合并到一对大的顶点/索引缓冲区中
    // 以此来定义每个子网格数据在缓冲区中所占的范围

    // 对合并顶点缓冲区中的每个物体的顶点偏移量进行缓存
    uint32_t boxVertexOffset = 0;
    uint32_t gridVertexOffset = boxVertexOffset + static_cast<uint32_t>(box.Vertices.size());

    // 对合并索引缓冲区中的每个物体的起始索引进行缓存
    uint32_t boxIndexOffset = 0;
    uint32_t gridIndexOffset = boxIndexOffset + static_cast<uint32_t>(box.Indices32.size());

    // 定义多个SubmeshGeometry结构体中包含了顶点/索引缓冲区内不同几何体的子网格数据
    SubmeshGeometry boxSubmesh;
    boxSubmesh.IndexCount = static_cast<uint32_t>(box.Indices32.size());
    boxSubmesh.StartIndexLocation = boxIndexOffset;
    boxSubmesh.BaseVertexLocation = boxVertexOffset;

    SubmeshGeometry gridSubmesh;
    gridSubmesh.IndexCount = static_cast<uint32_t>(grid.Indices32.size());
    gridSubmesh.StartIndexLocation = gridIndexOffset;
    gridSubmesh.BaseVertexLocation = gridVertexOffset;


    // 提取出所需的顶点元素，再将所有网格的顶点装进一个顶点缓冲区
    auto totalVertexCount = box.Vertices.size() + grid.Vertices.size();

    std::vector<FrameUtil::Vertex> vertices(totalVertexCount);

    uint32_t k = 0;

    for (size_t i = 0; i < box.Vertices.size(); ++i, ++k) {
        vertices[k].position = box.Vertices[i].Position;
        vertices[k].uv = box.Vertices[i].TexC;
    }

    for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k) {
        vertices[k].position = grid.Vertices[i].Position;
        // vertices[k].position.y = abs(sinf((1.0f / (rand() % 10)) * 3));
        vertices[k].uv = grid.Vertices[i].TexC;
    }

    std::vector<std::uint32_t> indices;

    indices.insert(indices.end(), box.Indices32.begin(), box.Indices32.end());
    indices.insert(indices.end(), grid.Indices32.begin(), grid.Indices32.end());

    const uint32_t vertexBufferByteSize = (uint32_t)vertices.size() * sizeof(FrameUtil::Vertex);
    const uint32_t indexBufferByteSize = (uint32_t)indices.size() * sizeof(std::uint32_t);

    auto geometry = std::make_unique<MeshGeometry>();

    geometry->Name = "ShapeGeometry";

    ThrowIfFailed(D3DCreateBlob(vertexBufferByteSize, &geometry->VertexBufferCPU));
    CopyMemory(geometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vertexBufferByteSize);

    ThrowIfFailed(D3DCreateBlob(indexBufferByteSize, &geometry->IndexBufferCPU));
    CopyMemory(geometry->IndexBufferCPU->GetBufferPointer(), indices.data(), indexBufferByteSize);

    geometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
    commandList.Get(), vertices.data(), vertexBufferByteSize, geometry->VertexBufferUploader);

    geometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
    commandList.Get(), indices.data(), indexBufferByteSize, geometry->IndexBufferUploader);

    geometry->VertexByteStride = sizeof(FrameUtil::Vertex);
    geometry->VertexBufferByteSize = vertexBufferByteSize;
    geometry->IndexFormat = DXGI_FORMAT_R32_UINT;
    geometry->IndexBufferByteSize = indexBufferByteSize;

    geometry->DrawArgs["Box"] = boxSubmesh;
    geometry->DrawArgs["Grid"] = gridSubmesh;

    geometries[geometry->Name] = std::move(geometry);
}

void shapesApp::buildRenderItems() {
    auto boxRenderItem = std::make_unique<FrameUtil::RenderItem>();

    XMStoreFloat4x4(&boxRenderItem->world, XMMatrixScaling(1.1f, 1.1f, 1.1f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f));
    boxRenderItem->objectConstantBufferIndex = 0;
    boxRenderItem->geometry = geometries["ShapeGeometry"].get();
    boxRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRenderItem->indexCount = boxRenderItem->geometry->DrawArgs["Box"].IndexCount;
    boxRenderItem->startIndexLocation = boxRenderItem->geometry->DrawArgs["Box"].StartIndexLocation;
    boxRenderItem->baseVertexLocation = boxRenderItem->geometry->DrawArgs["Box"].BaseVertexLocation;

    auto gridRenderItem = std::make_unique<FrameUtil::RenderItem>();

    XMStoreFloat4x4(&gridRenderItem->world, XMMatrixScaling(1.1f, 1.1f, 1.1f) * XMMatrixTranslation(0.0f, -1.0f, 0.0f));
    gridRenderItem->objectConstantBufferIndex = 1;
    gridRenderItem->geometry = geometries["ShapeGeometry"].get();
    gridRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRenderItem->indexCount = gridRenderItem->geometry->DrawArgs["Grid"].IndexCount;
    gridRenderItem->startIndexLocation = gridRenderItem->geometry->DrawArgs["Grid"].StartIndexLocation;
    gridRenderItem->baseVertexLocation = gridRenderItem->geometry->DrawArgs["Grid"].BaseVertexLocation;

    allRenderItems.push_back(std::move(boxRenderItem));
    allRenderItems.push_back(std::move(gridRenderItem));

    // 此演示程序中所有的渲染项都是非透明的
    for (auto& element : allRenderItems) {
        opaqueRenderItems.push_back(element.get());
    }
}

void shapesApp::updateObjectConstantBuffers() {
    // 用当前最新的worldViewProject矩阵来更新常量缓冲区
    auto currentObjectConstantBuffer = currentFrameResource->objectConstantBuffer.get();

    for (auto& element : allRenderItems) {
        // 只要常量发生了改变就得更新常量缓冲区内的数据
        // 而且要对每个帧资源都进行更新
        if (element->numFramesDirty > 0) {
            XMMATRIX world = XMLoadFloat4x4(&element->world);

            FrameUtil::ObjectConstants objectConstants;
            XMStoreFloat4x4(&objectConstants.world, XMMatrixTranspose(world));

            currentObjectConstantBuffer->CopyData(element->objectConstantBufferIndex, objectConstants);

            // 还需要对下一个FrameResource进行更新
            element->numFramesDirty--;
        }
    }
}

void shapesApp::updatePassConstantBuffers() {
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

    XMMATRIX newView = XMMatrixLookAtLH(position, target, up);
    
    //  | right.x up.x forward.x 0| 
    //  | right.y up.y forward.y 0|
    //  | right.z up.z forward.z 0|
    //  | tx      ty   tz        1|
    newView.r[0] = XMVectorSet(tempRight.x, tempUp.x, tempForward.x, 0.0f);
    newView.r[1] = XMVectorSet(tempRight.y, tempUp.y, tempForward.y, 0.0f);
    newView.r[2] = XMVectorSet(tempRight.z, tempUp.z, tempForward.z, 0.0f);
    
    float tx = -XMVectorGetX(XMVector3Dot(position, right));
    float ty = -XMVectorGetX(XMVector3Dot(position, up));
    float tz = -XMVectorGetX(XMVector3Dot(position, forward));

    newView.r[3] = XMVectorSet(tx, ty, tz, 1.0f);

    XMStoreFloat4x4(&view, newView);

    XMMATRIX newProjection = XMLoadFloat4x4(&projection);

    XMMATRIX newViewProjection = newView * newProjection;

    // 用当前最新的viewProject矩阵来更新常量缓冲区
    FrameUtil::PassConstants passConstants;

    XMStoreFloat4x4(&passConstants.viewProjection, XMMatrixTranspose(newViewProjection));
    passConstants.time = timer.TotalTime();

    currentFrameResource->passConstantBuffer->CopyData(0, passConstants);

    FrameUtil::MaterialConstants materialConstants;
    materialConstants.materialIndex = (uint32_t)(timer.TotalTime() * 15.0f) % textures.size();

    currentFrameResource->materialConstantBuffer->CopyData(0, materialConstants); 
}

void shapesApp::resetCommandList() {    
    // 重置命令列表为执行初始化命令做好准备工作
    ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
}

void shapesApp::executeCommandList() {
    // 执行初始化命令
    ThrowIfFailed(commandList->Close());
    ID3D12CommandList* commandLists[] = {commandList.Get()};
    commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void shapesApp::prepareFrameResourceSync() {
    currentFrameResource->fenceValue = ++currentFenceValue;
    commandQueue->Signal(fence.Get(), currentFenceValue);
}

void shapesApp::frameResourceSync() {
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

void shapesApp::initImGUI() 
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

void shapesApp::buildImGuiWidgets()
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
            ImGui::Checkbox("Wireframe", &isWireframe);

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

void shapesApp::updateImGui()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void shapesApp::renderImGui()
{
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}

void shapesApp::drawRenderItems(const std::vector<FrameUtil::RenderItem*>& renderItems) {
    uint32_t passConstantBufferIndex = frameResourcesCount * objectCount + currentFrameIndex;
    uint32_t materialConstantBufferIndex = frameResourcesCount * (objectCount + 1) + currentFrameIndex;

    uint32_t SRVDescriptorIndex = (objectCount + 1 + 1) * frameResourcesCount;

    CD3DX12_GPU_DESCRIPTOR_HANDLE SRVDescriptorHandle(CBVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), SRVDescriptorIndex, CBVSRVUAVDescriptorSize);

    for (size_t renderItemIndex = 0; renderItemIndex < renderItems.size(); renderItemIndex++) {
        auto renderItem = renderItems[renderItemIndex];

        commandList->IASetVertexBuffers(0, 1, &renderItem->geometry->VertexBufferView());
        commandList->IASetIndexBuffer(&renderItem->geometry->IndexBufferView());
        commandList->IASetPrimitiveTopology(renderItem->primitiveType);

        CD3DX12_GPU_DESCRIPTOR_HANDLE CBVDescriptorHandle(CBVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

        int32_t descriptorHeapIndex = currentFrameIndex * objectCount + static_cast<uint32_t>(renderItemIndex);

        CBVDescriptorHandle.Offset(descriptorHeapIndex, CBVSRVUAVDescriptorSize);

        commandList->SetGraphicsRootDescriptorTable(0, CBVDescriptorHandle);

        CD3DX12_GPU_DESCRIPTOR_HANDLE passCBVDescriptorHandle(CBVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

        passCBVDescriptorHandle.Offset(passConstantBufferIndex, CBVSRVUAVDescriptorSize);

        commandList->SetGraphicsRootDescriptorTable(1, passCBVDescriptorHandle);

        CD3DX12_GPU_DESCRIPTOR_HANDLE materialCBVDescriptorHandle(CBVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

        materialCBVDescriptorHandle.Offset(materialConstantBufferIndex, CBVSRVUAVDescriptorSize);

        commandList->SetGraphicsRootDescriptorTable(2, materialCBVDescriptorHandle);

        commandList->SetGraphicsRootDescriptorTable(3, SRVDescriptorHandle);

        commandList->SetGraphicsRootDescriptorTable(5, samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

        commandList->DrawIndexedInstanced(renderItem->indexCount, 1, renderItem->startIndexLocation, renderItem->baseVertexLocation, 0);
    }
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
        shapesApp app(hInstance, 1280, 720);
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