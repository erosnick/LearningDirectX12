#define WIN32_LEAN_AND_MEAN // 从Windows头中排除极少使用的资料
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

struct ObjectConstantBuffer {
    XMFLOAT4X4 world;
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

uint32_t calculateConstantBufferSize(uint32_t size) {
    return (size + 255) & (~255);
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

    ComPtr<IDXGIFactory5>               DXGIFactory; 
    ComPtr<IDXGIAdapter1>               adapter;
    ComPtr<ID3D12Device4>               device;
    ComPtr<IDXGISwapChain1>             temporarySwapChain;
    ComPtr<IDXGISwapChain3>             swapChain;
    ComPtr<ID3D12DescriptorHeap>        RTVDescriptorHeap;
    ComPtr<ID3D12DescriptorHeap>        CBVDescriptorHeap;
    ComPtr<ID3D12CommandQueue>          commandQueue;
    ComPtr<ID3D12CommandAllocator>      commandAllocator;
    ComPtr<ID3D12GraphicsCommandList>   commandList;
    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12PipelineState>         PSO;
    ComPtr<ID3D12Resource>              vertexBuffer;
    ComPtr<ID3D12Resource>              constantBuffer;
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

        #if defined(_DEBUG)
                {   //打开显示子系统的调试支持
                    ComPtr<ID3D12Debug> debugController;
                    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
                    {
                        debugController->EnableDebugLayer();
                        // 打开附加的调试支持
                        DXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
                    }
                }
        #endif

        // 1.创建IDXGIFactory
        CreateDXGIFactory2(DXGIFactoryFlags, IID_PPV_ARGS(DXGIFactory.GetAddressOf()));
        // 关闭Alt + Enter键切换全屏的功能，因为我们没有实现OnSize处理，所以先关闭
        ThrowIfFailed(DXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

        // 2.获取代表独显的的适配器(IDXGIAdapter)
        for (uint32_t adapterIndex = 0; 
            DXGIFactory->EnumAdapters1(adapterIndex, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; 
            adapterIndex++) {
            DXGI_ADAPTER_DESC1 adapterDesc = {};
            adapter->GetDesc1(&adapterDesc);

            if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                // 软件虚拟适配器，跳过
                continue;
            }

            // 3.检查适配器对12.1特性等级(Feature Level)的支持
            // 检查适配器对D3D12支持的兼容级别，这里直接要求支持12.1的能力，
            // 注意返回接口的那个参数被置为了nullptr，这样就不会实际创建一个
            // 设备了，也不用我们啰嗦的再调用release来释放接口。这也是一个
            // 重要的技巧，请记住！
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }

        // 4.实际进行设备创建(ID3D12Device)
        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(device.GetAddressOf())));

        // 5.创建命令队列(ID3D12CommandQueue)
        // struct D3D12_COMMAND_QUEUE_DESC
        // {
        //     D3D12_COMMAND_LIST_TYPE Type;
        //     INT Priority;
        //     D3D12_COMMAND_QUEUE_FLAGS Flags;
        //     UINT NodeMask;
        // };

        // enum D3D12_COMMAND_LIST_TYPE
        // {
        //     D3D12_COMMAND_LIST_TYPE_DIRECT	= 0,
        //     D3D12_COMMAND_LIST_TYPE_BUNDLE	= 1,
        //     D3D12_COMMAND_LIST_TYPE_COMPUTE	= 2,
        //     D3D12_COMMAND_LIST_TYPE_COPY	= 3,
        //     D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE	= 4,
        //     D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS	= 5,
        //     D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE	= 6
        // };
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;    // 表示“全能型”命令队列

        ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(commandQueue.GetAddressOf())));

        // 6.创建交换链(IDXGISwapChain)
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = backBufferCount;
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = backBufferFormt;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;

        ThrowIfFailed(DXGIFactory->CreateSwapChainForHwnd(
            commandQueue.Get(),   // 命令队列(For Direct3D 12 this is a pointer to a direct command queue)
            hWnd,                 // 窗口句柄
            &swapChainDesc,       // 交换链描述
            nullptr, 
            nullptr, 
            temporarySwapChain.GetAddressOf()   // 创建好的交换链
            ));

        ThrowIfFailed(temporarySwapChain.As(&swapChain));

        // 从IDXGISwapChain1获得高版本的IDXGISwapChain3
        backBufferIndex = swapChain->GetCurrentBackBufferIndex();

        // 7.创建RTV描述符堆(ID3D12DescriptorHeap)
        D3D12_DESCRIPTOR_HEAP_DESC RTVDescriptorHeapDesc = {};
        RTVDescriptorHeapDesc.NumDescriptors = backBufferCount;
        RTVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        RTVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(device->CreateDescriptorHeap(&RTVDescriptorHeapDesc, IID_PPV_ARGS(RTVDescriptorHeap.GetAddressOf())));

        D3D12_DESCRIPTOR_HEAP_DESC CBVDescriptorHeapDesc = {};
        CBVDescriptorHeapDesc.NumDescriptors = 1;
        CBVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        CBVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        ThrowIfFailed(device->CreateDescriptorHeap(&CBVDescriptorHeapDesc, IID_PPV_ARGS(CBVDescriptorHeap.GetAddressOf())));

        // 8.创建RTV描述符(Descriptor)
        // 获取RTV描述符大小
        RTVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE RTVDescriptorHandle(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        for (uint32_t backBufferIndex = 0; backBufferIndex < backBufferCount; backBufferIndex++) {
            ThrowIfFailed(swapChain->GetBuffer(backBufferIndex, IID_PPV_ARGS(renderTargets[backBufferIndex].GetAddressOf())));
            device->CreateRenderTargetView(renderTargets[backBufferIndex].Get(), nullptr, RTVDescriptorHandle);
            RTVDescriptorHandle.Offset(1, RTVDescriptorSize);
        }

        // 9.创建根签名(ID3D12RootSignature)
        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;

        CD3DX12_DESCRIPTOR_RANGE1 descriptorRange;

        descriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0);

        CD3DX12_ROOT_PARAMETER1 rootParameter;

        rootParameter.InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(1, &rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));

        ThrowIfFailed(device->CreateRootSignature(
            0, 
            signature->GetBufferPointer(), 
            signature->GetBufferSize(), 
            IID_PPV_ARGS(rootSignature.GetAddressOf())));

        // 10.编译&加载Shader(Shader在项目构建之前通过dxc进行了离线编译)
        // 这里只需加载编译好的二进制代码即可
        std::string vertexShader = loadShader("Shaders/vs.bin");
        std::string pixelShader = loadShader("Shaders/ps.bin");

        // 10.创建顶点输入布局(Input Layout)
        // struct D3D12_INPUT_ELEMENT_DESC
        // {
        //     LPCSTR SemanticName;
        //     UINT SemanticIndex;
        //     DXGI_FORMAT Format;
        //     UINT InputSlot;
        //     UINT AlignedByteOffset;
        //     D3D12_INPUT_CLASSIFICATION InputSlotClass;
        //     UINT InstanceDataStepRate;
        // }; 
        D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        D3D12_INPUT_LAYOUT_DESC inputLayout = {inputElementDesc, _countof(inputElementDesc)};

        // 11.创建渲染管线状态对象(ID3D12PipelineStateObject)
        D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc = {};
        graphicsPipelineStateDesc.InputLayout = inputLayout;
        graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
        graphicsPipelineStateDesc.VS = {vertexShader.data(), vertexShader.size()};
        graphicsPipelineStateDesc.PS = {pixelShader.data(), pixelShader.size()};
        // CD3DX12_RASTERIZER_DESC( CD3DX12_DEFAULT )
        // {
        //     FillMode = D3D12_FILL_MODE_SOLID;
        //     CullMode = D3D12_CULL_MODE_BACK;
        //     FrontCounterClockwise = FALSE;
        //     DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        //     DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        //     SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        //     DepthClipEnable = TRUE;
        //     MultisampleEnable = FALSE;
        //     AntialiasedLineEnable = FALSE;
        //     ForcedSampleCount = 0;
        //     ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        // }
        graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        // CD3DX12_BLEND_DESC( CD3DX12_DEFAULT )
        // {
        //     AlphaToCoverageEnable = FALSE;
        //     IndependentBlendEnable = FALSE;
        //     const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
        //     {
        //         FALSE,FALSE,
        //         D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        //         D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        //         D3D12_LOGIC_OP_NOOP,
        //         D3D12_COLOR_WRITE_ENABLE_ALL,
        //     };
        //     for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        //         RenderTarget[ i ] = defaultRenderTargetBlendDesc;
        // }
        graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        // CD3DX12_DEPTH_STENCIL_DESC( CD3DX12_DEFAULT )
        // {
        //     DepthEnable = TRUE;
        //     DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        //     DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        //     StencilEnable = FALSE;
        //     StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        //     StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        //     const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
        //     { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
        //     FrontFace = defaultStencilOp;
        //     BackFace = defaultStencilOp;
        // }
        // graphicsPipelineStateDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        graphicsPipelineStateDesc.DepthStencilState.DepthEnable = false;
        graphicsPipelineStateDesc.DepthStencilState.StencilEnable = false;
        graphicsPipelineStateDesc.SampleMask = UINT_MAX;
        graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        graphicsPipelineStateDesc.NumRenderTargets = 1;
        graphicsPipelineStateDesc.RTVFormats[0] = backBufferFormt;
        graphicsPipelineStateDesc.SampleDesc.Count = 1;
        graphicsPipelineStateDesc.SampleDesc.Quality = 0;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(PSO.GetAddressOf())));

        // 12.加载待渲染数据，创建顶点缓冲
        // 使用了三角形外接圆半径的形式参数化定义
        Vertex vertices[] = {
            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f * aspectRatio, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f * aspectRatio, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
       };

       const uint32_t vertexBufferSize = sizeof(vertices);

        // CD3DX12_HEAP_PROPERTIES( 
        //     D3D12_HEAP_TYPE type, 
        //     UINT creationNodeMask = 1, 
        //     UINT nodeMask = 1 )
        // {
        //     Type = type;
        //     CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        //     MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        //     CreationNodeMask = creationNodeMask;
        //     VisibleNodeMask = nodeMask;
        // }

        // CD3DX12_RESOURCE_DESC( 
        //     D3D12_RESOURCE_DIMENSION dimension,
        //     UINT64 alignment,
        //     UINT64 width,
        //     UINT height,
        //     UINT16 depthOrArraySize,
        //     UINT16 mipLevels,
        //     DXGI_FORMAT format,
        //     UINT sampleCount,
        //     UINT sampleQuality,
        //     D3D12_TEXTURE_LAYOUT layout,
        //     D3D12_RESOURCE_FLAGS flags )
        // {
        //     Dimension = dimension;
        //     Alignment = alignment;
        //     Width = width;
        //     Height = height;
        //     DepthOrArraySize = depthOrArraySize;
        //     MipLevels = mipLevels;
        //     Format = format;
        //     SampleDesc.Count = sampleCount;
        //     SampleDesc.Quality = sampleQuality;
        //     Layout = layout;
        //     Flags = flags;
        // }

        // CD3DX12_RESOURCE_DESC Buffer( 
        // UINT64 width,
        // D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
        // UINT64 alignment = 0 )
        // {
        //     return CD3DX12_RESOURCE_DESC( D3D12_RESOURCE_DIMENSION_BUFFER, alignment, width, 1, 1, 1, 
        //         DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags );
        // }
        D3D12_HEAP_PROPERTIES vertexBufferUploadHeapProperties = {D3D12_HEAP_TYPE_UPLOAD};

        D3D12_RESOURCE_DESC vertexBufferDesc = {};
        vertexBufferDesc.Alignment = 0;
        vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vertexBufferDesc.Width = vertexBufferSize;
        vertexBufferDesc.Height = 1;
        vertexBufferDesc.MipLevels = 1;
        vertexBufferDesc.DepthOrArraySize = 1;
        vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        vertexBufferDesc.SampleDesc.Count = 1;
        vertexBufferDesc.SampleDesc.Quality = 0;
        vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        vertexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ThrowIfFailed(device->CreateCommittedResource(
            &vertexBufferUploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &vertexBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(vertexBuffer.GetAddressOf())));

        byte* vertexData = nullptr;

        CD3DX12_RANGE readRange(0, 0);

        ThrowIfFailed(vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexData)));
        memcpy_s(vertexData,vertexBufferSize, vertices, vertexBufferSize);
        vertexBuffer->Unmap(0, nullptr);

        D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(Vertex);
        vertexBufferView.SizeInBytes = vertexBufferSize;

        D3D12_HEAP_PROPERTIES constantBufferUploadHeapProperties = {D3D12_HEAP_TYPE_UPLOAD};

        uint32_t constantBufferSize = calculateConstantBufferSize(sizeof(ObjectConstantBuffer));

        D3D12_RESOURCE_DESC constantBufferDesc = {};
        constantBufferDesc.Alignment = 0;
        constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        constantBufferDesc.Width = constantBufferSize;
        constantBufferDesc.Height = 1;
        constantBufferDesc.MipLevels = 1;
        constantBufferDesc.DepthOrArraySize = 1;
        constantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        constantBufferDesc.SampleDesc.Count = 1;
        constantBufferDesc.SampleDesc.Quality = 0;
        constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        constantBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ThrowIfFailed(device->CreateCommittedResource(
            &constantBufferUploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &constantBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(constantBuffer.GetAddressOf())));

        byte* mappedData = nullptr;

        ThrowIfFailed(constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

        CBVSRVUAVDescritproSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE CBVDescriptorHandle(CBVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        D3D12_GPU_VIRTUAL_ADDRESS constantBufferAddress = constantBuffer->GetGPUVirtualAddress();

        D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
        constantBufferViewDesc.BufferLocation = constantBufferAddress;
        constantBufferViewDesc.SizeInBytes = constantBufferSize;

        device->CreateConstantBufferView(&constantBufferViewDesc, CBVDescriptorHandle);

        // 13.创建命令分配器(ID3D12CommandAllocator)
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocator.GetAddressOf())));

        // 14.创建命令列表(ID3D12CommandList)
        ThrowIfFailed(device->CreateCommandList(
            0, 
            D3D12_COMMAND_LIST_TYPE_DIRECT, 
            commandAllocator.Get(), 
            nullptr, 
            IID_PPV_ARGS(commandList.GetAddressOf())));

        // 15.创建围栏(ID3D12Fence)
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));

        fenceValue = 1;

        // 16.创建一个Event同步对象，用于等待围栏事件通知
        fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

        if (fenceEvent == nullptr) {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // 17.设置视口(D3D12_VIEWPORT)
        CD3DX12_VIEWPORT viewport(0.0f, 0.0f, 
                                  static_cast<float>(width), 
                                  static_cast<float>(height));
        // 18.设置裁剪矩形
        CD3DX12_RECT scissorRect(0, 0, 
                                 static_cast<long>(width), 
                                 static_cast<long>(height));

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        LARGE_INTEGER startTime;
        LARGE_INTEGER endTime;
        LARGE_INTEGER elapsedTime;

        float frameTime = 0.0f;

        ObjectConstantBuffer constantBufferData;

        float rotateAngle = 0.0f;

        while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                QueryPerformanceCounter(&startTime);

                XMMATRIX world =  XMMatrixRotationZ(rotateAngle);

                XMStoreFloat4x4(&constantBufferData.world, XMMatrixTranspose(world));

                memcpy_s(mappedData, sizeof(ObjectConstantBuffer), &constantBufferData, sizeof(ObjectConstantBuffer));

                rotateAngle += frameTime;

                // 通过资源屏障判定缓冲已经切换完毕可以开始渲染了
                // CD3DX12_RESOURCE_BARRIER Transition(
                //     _In_ ID3D12Resource* pResource,
                //     D3D12_RESOURCE_STATES stateBefore,
                //     D3D12_RESOURCE_STATES stateAfter,
                //     UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                //     D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE)
                // {
                //     CD3DX12_RESOURCE_BARRIER result;
                //     ZeroMemory(&result, sizeof(result));
                //     D3D12_RESOURCE_BARRIER &barrier = result;
                //     result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                //     result.Flags = flags;
                //     barrier.Transition.pResource = pResource;
                //     barrier.Transition.StateBefore = stateBefore;
                //     barrier.Transition.StateAfter = stateAfter;
                //     barrier.Transition.Subresource = subresource;
                //     return result;
                // }
                D3D12_RESOURCE_BARRIER resourceBarrierBegin = {};
                resourceBarrierBegin.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                resourceBarrierBegin.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                resourceBarrierBegin.Transition.pResource = renderTargets[backBufferIndex].Get();
                resourceBarrierBegin.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
                resourceBarrierBegin.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
                resourceBarrierBegin.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                commandList->ResourceBarrier(1, &resourceBarrierBegin);

                commandList->RSSetViewports(1, &viewport);
                commandList->RSSetScissorRects(1, &scissorRect);

                commandList->SetPipelineState(PSO.Get());

                ID3D12DescriptorHeap* descriptorHeaps[] = {CBVDescriptorHeap.Get()};
                commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
                commandList->SetGraphicsRootSignature(rootSignature.Get());
                commandList->SetGraphicsRootDescriptorTable(0, CBVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

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
                D3D12_RESOURCE_BARRIER resourceBarrierEnd = {};
                resourceBarrierEnd.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                resourceBarrierEnd.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                resourceBarrierEnd.Transition.pResource = renderTargets[backBufferIndex].Get();
                resourceBarrierEnd.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                resourceBarrierEnd.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
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

                // 看命令有没有真正执行到围栏标记的位置，没有就利用事件去等待，
                // 注意使用的是命令队列对象的指针
                if (fence->GetCompletedValue() < currentFenceValue) {
                    ThrowIfFailed(fence->SetEventOnCompletion(currentFenceValue, fenceEvent));
                    WaitForSingleObject(fenceEvent, INFINITE);
                }

                // 到这里说明一个命令队列完整的执行完了，在这里就代表我们的
                // 一帧已经渲染完了，接着准备执行下一帧，获得新的后备缓冲序号，
                // 因为Present真正的完成时后备缓冲的序号就更新了
                backBufferIndex = swapChain->GetCurrentBackBufferIndex();

                // 命令分配器Reset一下
                ThrowIfFailed(commandAllocator->Reset());

                // Reset命令列表，并重新指定命令分配器和PSO对象
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

    constantBuffer->Unmap(0, nullptr);

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