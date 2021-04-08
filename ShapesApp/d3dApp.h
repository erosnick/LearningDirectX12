#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include "Common/d3dx12.h"

#include <stdint.h>
#include <exception>
#include <vector>
#include <string>

#include "Common/GameTimer.h"
#include "Common/FrameResources.h"

using namespace Microsoft::WRL;

class d3dApp {
public:

    d3dApp(const d3dApp& rhs) = delete;
    d3dApp& operator=(const d3dApp& rhs) = delete;

    static d3dApp* getApp();    

    d3dApp(HINSTANCE inInstance, const uint32_t inWindowWidth, const uint32_t inWindowHeight);

    virtual LRESULT msgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    virtual bool initialize();

    int32_t getWindowWidth() const { return windowWidth; }

    int32_t getWindowHeight() const { return windowHeight; }

    float aspectRatio() const;

    int32_t run();

protected:

    bool createMainWindow();

    bool initDirect3D();

	virtual void onResize(); 
	virtual void update(float delta) = 0;
    virtual void draw(float delta) = 0;

	// Convenience overrides for handling mouse input.
	virtual void onMouseDown(WPARAM btnState, int32_t x, int32_t y);
	virtual void onMouseUp(WPARAM btnState, int32_t x, int32_t y);
	virtual void onMouseMove(WPARAM btnState, int32_t x, int32_t y);

    virtual void onKeyDown(WPARAM btnState);
    virtual void onKeyUp(WPARAM btnState);

    void enableDebugLayer();
    void logOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);
    void logAdapterOutputs(IDXGIAdapter1* adapter);
    void calculateFrameStats();
    void logAdapters(std::vector<IDXGIAdapter1*>& adapterList);
    void createDXGIFactory();
    void createDevice();
    void checkMSAASupport();
    void createFence();
    void createCommandObjects();
    void createSwapChain();
    void createRTVAndDSVDescriptorHeaps();
    void createRenderTargetViews();
    void createDepthStencilView();

    ID3D12Resource* currentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView();
    void setupViewport();


protected:

    uint32_t RTVDescriptorSize = 0;           // Render Target View
    uint32_t DSVDescriptorSize = 0;           // Depth Stencil View
    uint32_t CBVSRVUAVDescriptorSize = 0;     // Constant Buffer View, Shader Resource View, Unordered Access View

    uint32_t windowWidth = 1280;
    uint32_t windowHeight = 720;

    uint32_t MSAAQuality = 0;

    static const uint32_t frameBackBufferCount = 3;
    uint32_t currentFrameBackBufferIndex = 0;

    UINT64 currentFenceValue= 0;

    uint32_t DXGIFactoryFlags = 0;

    D3D12_VIEWPORT viewport;
    D3D12_RECT scissorRect;

    bool MSAA4X = false;
    bool appPaused = false;
    bool minimized = false;
    bool maximized = false;
    bool resizing = false;

    HWND mainWindow;

    HINSTANCE mainInstance;

    std::wstring windowTitle = L"D3D12";

    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthStencilBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    ComPtr<ID3D12Device> device = nullptr;
    ComPtr<IDXGIFactory5> DXGIFactory = nullptr;

    ComPtr<ID3D12Fence> fence = nullptr;

    ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12CommandAllocator> commandAllocator;

    ComPtr<IDXGISwapChain> temporarySwapChain = nullptr;
    ComPtr<IDXGISwapChain4> swapChain = nullptr;

    ComPtr<ID3D12DescriptorHeap> RTVDescriptorHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> DSVDescriptorHeap = nullptr;

    ComPtr<ID3D12Resource> frameBackBuffers[frameBackBufferCount];
    ComPtr<ID3D12Resource> depthStencilBuffer = nullptr;

    static d3dApp* app;

    GameTimer timer;
};