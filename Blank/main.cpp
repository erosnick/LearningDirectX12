#define WIN32_LEAN_AND_MEAN // 从Windows头中排除极少使用的资料
#include <Windows.h>
#include <wrl.h>
#include <string>
#include <comdef.h>

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

    HWND hWnd = nullptr;

    MSG msg = {};

    float aspectRatio = (float)width / height;

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};

    uint64_t fenceValue = 0;
    HANDLE fenceEvent = nullptr;

    CD3DX12_VIEWPORT viewprot(0.0f, 0.0f, 
                              static_cast<float>(width), 
                              static_cast<float>(height));
            
    CD3DX12_RECT scissorRect(0, 0, 
                             static_cast<long>(width), 
                             static_cast<long>(height));

    ComPtr<IDXGIFactory5>           DXGIFactory; 
    ComPtr<IDXGIAdapter1>           adapter;
    ComPtr<ID3D12Device4>           device;
    ComPtr<IDXGISwapChain1>         temporarySwapChain;
    ComPtr<IDXGISwapChain3>         swapChain;
    ComPtr<ID3D12DescriptorHeap>    RTVDescriptorHeap;
    ComPtr<ID3D12CommandQueue>      commandQueue;
    ComPtr<ID3D12CommandAllocator>  commandAllocator;
    ComPtr<ID3D12RootSignature>     rootSignature;
    ComPtr<ID3D12PipelineState>     PSO;
    ComPtr<ID3D12Resource>          vertexBuffer;
    ComPtr<ID3D12Resource>          RenderTargets[backBufferCount];
    ComPtr<ID3D12Fence>             fence;

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
        // 关闭Alt + Enter键切换全屏的功能，因为我们没有实现OnSize处理，所以先关闭
        ThrowIfFailed(DXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

        // 2.获取代表独显的的适配器
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

        // 4.实际进行设备创建
        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(device.GetAddressOf())));

        // 5.创建命令队列
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

        // 6.创建交换链
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = backBufferCount;
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Count = 0;

        ThrowIfFailed(DXGIFactory->CreateSwapChainForHwnd(
            device.Get(),   // 设备指针
            hWnd,           // 窗口句柄
            &swapChainDesc, // 交换链描述
            nullptr, 
            nullptr, 
            temporarySwapChain.GetAddressOf()   // 创建好的交换链
            ));

        ThrowIfFailed(temporarySwapChain.As(&swapChain));

        // 从IDXGISwapChain1获得高版本的IDXGISwapChain3
        backBufferIndex = swapChain->GetCurrentBackBufferIndex();

        while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {

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