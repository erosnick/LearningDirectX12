﻿#define WIN32_LEAN_AND_MEAN // 从Windows头中排除极少使用的资料
#include <Windows.h>
#include <wrl.h>
#include <string>
#include <comdef.h>
#include <fstream>

#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxcapi.h>

using namespace Microsoft::WRL;
using namespace DirectX;

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

#include "Common/d3dx12.h"

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};

class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber)
    : ErrorCode(hr),
      FunctionName(functionName),
      Filename(filename),
      LineNumber(lineNumber) {
    }

    std::wstring DxException::ToString()const
    {
        // Get the string description of the error code.
        _com_error err(ErrorCode);
        std::wstring msg = err.ErrorMessage();

        return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
    }

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring Filename;
    int LineNumber = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

ComPtr<IDxcBlob> compileShader(const std::wstring& sourceName, 
                                        const std::wstring& entryPoint, 
                                        const std::wstring& targetProfile) {
    HRESULT hr = S_OK;

    ComPtr<IDxcLibrary> library;
    hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(library.GetAddressOf()));

    ThrowIfFailed(hr);

    ComPtr<IDxcCompiler2> compiler;
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.GetAddressOf()));

    ThrowIfFailed(hr);

    uint32_t codePage = CP_UTF8;

    ComPtr<IDxcBlobEncoding> sourceBlob;
    hr = library->CreateBlobFromFile(sourceName.c_str(), &codePage, &sourceBlob);

    ThrowIfFailed(hr);
    
    // 基本上可以将所有的参数映射到D3DCompile上
    const wchar_t* args[] = {
		L"-Zpr",			//Row-major matrices
		L"-WX",				//Warnings as errors
#ifdef _DEBUG
		L"-Zi",				//Debug info
		L"-Qembed_debug",	//Embed debug info into the shader
		L"-Od",				//Disable optimization
#else
		L"-O3",				//Optimization level 3
#endif
	};

    ComPtr<IDxcOperationResult> result;

    hr = compiler->Compile(
        sourceBlob.Get(),
        sourceName.c_str(),
        entryPoint.c_str(),
        targetProfile.c_str(),
        // &args[0], _countof(args),
        nullptr, 0,
        nullptr, 0,
        nullptr,
        &result);

    if (SUCCEEDED(hr)) {
        result->GetStatus(&hr);
    }

    // Handle compilation error...
    if (FAILED(hr)) {
        if (result) {
            ComPtr<IDxcBlobEncoding> errorsBlob;
            hr = result->GetErrorBuffer(&errorsBlob);
            if (SUCCEEDED(hr) && errorsBlob) {
                wprintf(L"Compilatoin failed with errors:\n%hs\n", (const char*)errorsBlob->GetBufferPointer());
                ::OutputDebugStringA((char*)errorsBlob->GetBufferPointer());
            }
        }
    }

    ComPtr<IDxcBlob> code;
    result->GetResult(&code);

    return code;
}

void loadShader(const std::string& binaryName, std::string& shaderData) {
    std::ifstream file(binaryName, std::ios::in | std::ios::binary);
    
    // 移动文件指针到文件末尾
    file.seekg(0, std::ios_base::end);

    // 获取文件大小(byte)
    std::streampos length = file.tellg();

    // 移动文件指针到文件开始
    file.seekg(0, std::ios_base::beg);

    shaderData.resize(length);

    file.read(&shaderData[0], length);

    // 读取字节数
    std::streamsize bytes = file.gcount();

    file.close();
}

std::string loadShader(const std::string& binaryName) {
    std::string shaderData;

    loadShader(binaryName, shaderData);

    return shaderData;
}

const wchar_t* WINDOW_CLASS_NAME = L"Game Window Class";
const wchar_t* WINDOW_TITLE = L"DirectX 12 Sample";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
    // 创建控制台程序
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONIN$", "r+t", stdin);
    freopen_s(&stream, "CONOUT$", "w+t", stdout);

	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    const uint32_t backBufferCount = 3;

    uint32_t width = 1280;
    uint32_t height = 720;
    uint32_t backBufferIndex = 0;
    uint32_t frame = 0;

    uint32_t DXGIFactoryFlags = 0;
    uint32_t RTVDescriptorSize = 0;
    uint32_t CBVSRVUAVDescritproSize = 0;

    HWND hWnd = nullptr;

    MSG msg = {};

    float aspectRatio = (float)width / height;

    DXGI_FORMAT backBufferFormt = DXGI_FORMAT_R8G8B8A8_UNORM;

    uint64_t fenceValue = 0;
    HANDLE fenceEvent = nullptr;

    ComPtr<IDXGIFactory5>           DXGIFactory; 
    ComPtr<IDXGIAdapter1>           adapter;
    ComPtr<ID3D12Device4>           device;
    ComPtr<IDXGISwapChain1>             temporarySwapChain;
    ComPtr<IDXGISwapChain3>             swapChain;
    ComPtr<ID3D12DescriptorHeap>        RTVDescriptorHeap;
    ComPtr<ID3D12CommandQueue>          commandQueue;
    ComPtr<ID3D12CommandAllocator>      commandAllocator;
    ComPtr<ID3D12GraphicsCommandList>   commandList;
    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12PipelineState>         PSO;
    ComPtr<ID3D12Resource>              vertexBuffer;
    ComPtr<ID3D12Resource>              renderTargets[backBufferCount];
    ComPtr<ID3D12Fence>                 fence;

    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_GLOBALCLASS;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);		//防止无聊的背景重绘
    wcex.lpszClassName = WINDOW_CLASS_NAME;
    RegisterClassEx(&wcex);

    hWnd = CreateWindowW(WINDOW_CLASS_NAME, 
                         WINDOW_TITLE, 
                         WS_OVERLAPPED | WS_SYSMENU, 
                         CW_USEDEFAULT, 
                         0, 
                         width, 
                         height, 
                         nullptr, 
                         nullptr, 
                         hInstance, 
                         nullptr);
 
    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, showCmd);
    UpdateWindow(hWnd);

    try {

        // 1.创建IDXGIFactory
        CreateDXGIFactory2(DXGIFactoryFlags, IID_PPV_ARGS(DXGIFactory.GetAddressOf()));

        // 2.枚举适配器，获得性能最强的
        uint32_t adapterIndex = 0;
        DXGI_ADAPTER_DESC1 adapterDesc = {};
        while (DXGIFactory->EnumAdapters1(adapterIndex, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND) {
            adapter->GetDesc1(&adapterDesc);

            // 软件虚拟适配器，跳过
            if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            // 3.检测适配器对D3D12.1特性等级(Feature Level)的支持情况
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr))) {
                break;
            }
            
            adapterIndex++;
        }

        // 4.实际进行设备的创建(ID3D12Device)
        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(device.GetAddressOf())));

        // 5.创建命令列表(ID3D12CommandQueue)
        D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
        commandQueueDesc.NodeMask = 0;
        commandQueueDesc.Priority = 0;
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(commandQueue.GetAddressOf())));

        // 6.创建交换链(ID3D12SwapChain)
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapChainDesc.Format = backBufferFormt;
        swapChainDesc.BufferCount = backBufferCount;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        ThrowIfFailed(DXGIFactory->CreateSwapChainForHwnd(
            commandQueue.Get(), 
            hWnd, 
            &swapChainDesc, 
            nullptr, 
            nullptr, 
            temporarySwapChain.GetAddressOf()));

        // 从ID3D12SwapChain1接口查询高版本的ID3D12SwapChain3接口
        ThrowIfFailed(temporarySwapChain.As(&swapChain));

        // 7.创建RTV描述符堆(ID3D12DescriptorHeap)
        D3D12_DESCRIPTOR_HEAP_DESC RTVDescriptorHeapDesc;
        RTVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        RTVDescriptorHeapDesc.NumDescriptors = backBufferCount;
        RTVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        RTVDescriptorHeapDesc.NodeMask = 0;

        ThrowIfFailed(device->CreateDescriptorHeap(&RTVDescriptorHeapDesc, IID_PPV_ARGS(RTVDescriptorHeap.GetAddressOf())));

        // 8.创建RTV描述符(Render Targets)
        // 获取RTV描述符大小
        RTVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE RTVCPUDescriptorHandle(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        for(uint32_t backBufferIndex = 0; backBufferIndex < backBufferCount; backBufferIndex++) {
            ThrowIfFailed(swapChain->GetBuffer(backBufferIndex, IID_PPV_ARGS(renderTargets[backBufferIndex].GetAddressOf())));
            device->CreateRenderTargetView(renderTargets[backBufferIndex].Get(), nullptr, RTVCPUDescriptorHandle);
            RTVCPUDescriptorHandle.Offset(1, RTVDescriptorSize);
        }

        // 9.创建根签名(ID3D12RootSignature)
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> rootSignatureData;
        ComPtr<ID3DBlob> error;

        ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, rootSignatureData.GetAddressOf(), error.GetAddressOf()));

        ThrowIfFailed(device->CreateRootSignature(
            0,
            rootSignatureData->GetBufferPointer(), 
            rootSignatureData->GetBufferSize(), 
            IID_PPV_ARGS(rootSignature.GetAddressOf())));

        // 10.编译和加载Shader(使用dxc进行离线编译，然后加载编译好的二进制字节码)
        std::string vertexShaderData = loadShader("Shaders/vs.bin");
        std::string pixelShaderData = loadShader("Shaders/ps.bin");

        D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

        D3D12_INPUT_LAYOUT_DESC inputLayout = {inputElementDesc, _countof(inputElementDesc)};

        D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
        PSODesc.InputLayout = inputLayout;
        PSODesc.pRootSignature = rootSignature.Get();
        PSODesc.VS = {vertexShaderData.data(), vertexShaderData.size()};
        PSODesc.PS = {pixelShaderData.data(), pixelShaderData.size()};
        PSODesc.NodeMask = 0;
        PSODesc.NumRenderTargets = 1;
        PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        PSODesc.DepthStencilState.DepthEnable = false;
        PSODesc.DepthStencilState.StencilEnable = false;
        PSODesc.SampleDesc.Count = 1;
        PSODesc.SampleDesc.Quality = 0;
        PSODesc.SampleMask = UINT_MAX;
        PSODesc.RTVFormats[0] = backBufferFormt;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(PSO.GetAddressOf())));

        // 12.加载待渲染数据，创建顶点缓冲
        // 使用了三角形外接圆半径的形式参数化定义
        Vertex vertices[] = {
            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f * aspectRatio, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f * aspectRatio, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
       };

       const uint32_t vertexBufferSize = sizeof(vertices);
       
       CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);

       D3D12_RESOURCE_DESC resourceDesc;
       resourceDesc.Alignment = 0;
       resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
       resourceDesc.Width = vertexBufferSize;
       resourceDesc.Height = 1;
       resourceDesc.MipLevels = 1;
       resourceDesc.DepthOrArraySize = 1;
       resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
       resourceDesc.SampleDesc.Count = 1;
       resourceDesc.SampleDesc.Quality = 0;
       resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
       resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

       ThrowIfFailed(device->CreateCommittedResource(
           &heapProperties,
           D3D12_HEAP_FLAG_NONE,
           &resourceDesc,
           D3D12_RESOURCE_STATE_GENERIC_READ,
           nullptr,
           IID_PPV_ARGS(vertexBuffer.GetAddressOf())));

        byte* vertexData = nullptr;

        CD3DX12_RANGE readRange(0, 0);

        ThrowIfFailed(vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexData)));
        memcpy_s(vertexData, vertexBufferSize, vertices, vertexBufferSize);
        vertexBuffer->Unmap(0, nullptr);

        D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(Vertex);
        vertexBufferView.SizeInBytes = vertexBufferSize;

        // 13.创建命令分配器(ID3D12CommandAllocator)
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocator.GetAddressOf())));

        // 14.创建命令列表(ID3D12CommandList)
        ThrowIfFailed(device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            commandAllocator.Get(),
            PSO.Get(),
            IID_PPV_ARGS(commandList.GetAddressOf())));

        // 15.创建围栏(ID3D12Fence)
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));

        fenceValue = 1;

        // 16.创建一个Event同步对象，用于等待围栏事件通知
        fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

        if (fenceEvent == nullptr) {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // 16.创建视口(D3D12_VIEWPORT)
        CD3DX12_VIEWPORT viewport(0.0f, 0.0f,
                                  static_cast<float>(width),
                                  static_cast<float>(height));

        // 17.创建裁剪矩形
        CD3DX12_RECT scissorRect(0, 0,
                                 static_cast<long>(width),
                                 static_cast<long>(height));

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        LARGE_INTEGER startTime;
        LARGE_INTEGER endTime;
        LARGE_INTEGER elapsedTime;
        float frameTime;

        while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                QueryPerformanceCounter(&startTime);

                commandList->SetGraphicsRootSignature(rootSignature.Get()); 
                commandList->RSSetViewports(1, &viewport);
                commandList->RSSetScissorRects(1, &scissorRect);

                D3D12_RESOURCE_BARRIER resourceBarrierBegin;

                resourceBarrierBegin.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                resourceBarrierBegin.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                resourceBarrierBegin.Transition.pResource = renderTargets[backBufferIndex].Get();
                resourceBarrierBegin.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
                resourceBarrierBegin.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; 
                resourceBarrierBegin.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                commandList->ResourceBarrier(1, &resourceBarrierBegin);

                CD3DX12_CPU_DESCRIPTOR_HANDLE RTVDescriptorHandle(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

                RTVDescriptorHandle.Offset(backBufferIndex, RTVDescriptorSize);

                // 设置渲染目标
                commandList->OMSetRenderTargets(1, &RTVDescriptorHandle, false, nullptr);

                // 继续记录命令，并真正开始新一帧的渲染
                const float clearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};

                commandList->ClearRenderTargetView(RTVDescriptorHandle, clearColor, 0, nullptr);
                commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

                commandList->DrawInstanced(3, 1, 0, 0);

                // 又是一个资源屏障，用于确定渲染已经结束可以提交画面去显示了
                D3D12_RESOURCE_BARRIER resourceBarrierEnd;

                resourceBarrierEnd.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                resourceBarrierEnd.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                resourceBarrierEnd.Transition.pResource = renderTargets[backBufferIndex].Get();
                resourceBarrierEnd.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                resourceBarrierEnd.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
                resourceBarrierEnd.Transition.pResource = renderTargets[backBufferIndex].Get();;
                resourceBarrierEnd.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                commandList->ResourceBarrier(1, &resourceBarrierEnd);

                // 关闭命令列表，可以去执行了
                ThrowIfFailed(commandList->Close());

                // 执行命令列表
                ID3D12CommandList* commandLists[] = {commandList.Get()};
                commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

                // 提交画面
                ThrowIfFailed(swapChain->Present(1, 0));

                uint64_t currentFenceValue = fenceValue;

                // 开始同步GPU与CPU执行，先记录围栏标记值
                ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue));
                fenceValue++;

                if (fence->GetCompletedValue() < currentFenceValue) {
                    ThrowIfFailed(fence->SetEventOnCompletion(currentFenceValue, fenceEvent));
                    WaitForSingleObject(fenceEvent, INFINITE);
                }

                backBufferIndex = swapChain->GetCurrentBackBufferIndex();

                ThrowIfFailed(commandAllocator->Reset());

                ThrowIfFailed(commandList->Reset(commandAllocator.Get(), PSO.Get()));

                QueryPerformanceCounter(&endTime);

                elapsedTime.QuadPart = endTime.QuadPart - startTime.QuadPart;
                elapsedTime.QuadPart *= 1000000;
                elapsedTime.QuadPart /= frequency.QuadPart;
                frameTime = static_cast<float>(elapsedTime.QuadPart) / 1000000.0f;
            }
        }
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
    
    // 程序退出之前关闭标准输入输出
    fclose(stdin);
    fclose(stdout);

    return static_cast<int32_t>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}