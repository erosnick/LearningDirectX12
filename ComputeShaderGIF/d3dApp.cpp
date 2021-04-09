#include "d3dApp.h"
#include <WindowsX.h>

#include <iostream>
#include <exception>
#include <string>

#include "Common/d3dUtil.h"

d3dApp* d3dApp::app = nullptr;
d3dApp* d3dApp::getApp()
{
    return app;
}

d3dApp::d3dApp(HINSTANCE inInstance, const uint32_t inWindowWidth, const uint32_t inWindowHeight) 
: windowWidth(inWindowWidth),
  windowHeight(inWindowHeight),
  mainInstance(inInstance) {

    // Only one D3DApp can be constructed.
    assert(app == nullptr);
    app = this;
}

LRESULT CALLBACK
mainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
    return d3dApp::getApp()->msgProc(hwnd, msg, wParam, lParam);
}

bool d3dApp::initialize() {

    if (!createMainWindow()) {
        return false;
    }

    if (!initDirect3D()) {
        return false;
    }

    // Do the initial resize code.
    onResize();

    return true;
}

LRESULT d3dApp::msgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch( msg ) {
	// WM_ACTIVATE is sent when the window is activated or deactivated.  
	// We pause the game when the window is deactivated and unpause it 
	// when it becomes active.  
	case WM_ACTIVATE:
		if( LOWORD(wParam) == WA_INACTIVE ) {
			appPaused = true;
			timer.Stop();
		}
		else {
			appPaused = false;
			timer.Start();
		}
		return 0;

	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		windowWidth  = LOWORD(lParam);
		windowHeight = HIWORD(lParam);
		if( device ) {
			if( wParam == SIZE_MINIMIZED ) {
				appPaused = true;
				minimized = true;
				maximized = false;
			}
			else if( wParam == SIZE_MAXIMIZED ) {
				appPaused = false;
				minimized = false;
				maximized = true;
				onResize();
			}
			else if( wParam == SIZE_RESTORED ) {
				
				// Restoring from minimized state?
				if( minimized ) {
					appPaused = false;
					minimized = false;
					onResize();
				}

				// Restoring from maximized state?
				else if( maximized ) {
					appPaused = false;
					maximized = false;
					onResize();
				}
				else if( resizing ) {
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
                // API call such as SetWindowPos or mSwapChain->SetFullscreenState.{
				else {
					onResize();
				}
			}
		}
		return 0;

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		appPaused = true;
		resizing  = true;
		timer.Stop();
		return 0;

	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		appPaused = false;
		resizing  = false;
		timer.Start();
		onResize();
		return 0;
 
	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

	// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		onMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		onMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		onMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
    case WM_KEYDOWN:
        onKeyDown(wParam);
        break;
    case WM_KEYUP:
        if(wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if((int)wParam == VK_F2)
            MSAA4X = !MSAA4X;

        onKeyUp(wParam);
        break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool d3dApp::createMainWindow()
{
	WNDCLASS wc;
	wc.style         = CS_HREDRAW | CS_VREDRAW;  //去掉Redraw类型
	wc.lpfnWndProc   = mainWndProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = mainInstance;
	wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);  //防止无聊的背景重绘
	wc.lpszMenuName  = 0;
	wc.lpszClassName = L"MainWnd";

	if( !RegisterClass(&wc) )
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, static_cast<long>(windowWidth), static_cast<long>(windowHeight) };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width  = R.right - R.left;
	int height = R.bottom - R.top;

    // WS_OVERLAPPEDWINDOW会触发WM_SIZE消息
    // WS_OVERLAPPED则不会
	mainWindow = CreateWindow(L"MainWnd", windowTitle.c_str(), 
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mainInstance, 0); 
	if( !mainWindow )
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mainWindow, SW_SHOW);
	UpdateWindow(mainWindow);

	return true;
}

bool d3dApp::initDirect3D() {
    createDXGIFactory();
    createDevice();
    createFence();
    createCommandObjects();
    createSwapChain();
    createRTVAndDSVDescriptorHeaps();

    return true;
}

void d3dApp::onResize() {
    assert(device);
	assert(swapChain);

    assert(commandAllocator);

    ThrowIfFailed(graphicsCommandList->Reset(commandAllocator.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < frameBackBufferCount; ++i) {
        frameBackBuffers[i].Reset();
    }
		
    depthStencilBuffer.Reset();
	
	// Resize the swap chain.
    ThrowIfFailed(swapChain->ResizeBuffers(
		frameBackBufferCount, 
		windowWidth, windowHeight, 
		backBufferFormat, 
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    createRenderTargetViews();
    createDepthStencilView();
	
	// Update the viewport transform to cover the client area.
    setupViewport();

    // Execute the resize commands.
    ThrowIfFailed(graphicsCommandList->Close());
    ID3D12CommandList* commandLists[] = { graphicsCommandList.Get() };
    graphicsCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    currentFrameBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
}

// Convenience overrides for handling mouse input.
void d3dApp::onMouseDown(WPARAM btnState, int32_t x, int32_t y) {

}

void d3dApp::onMouseUp(WPARAM btnState, int32_t x, int32_t y) {

}

void d3dApp::onMouseMove(WPARAM btnState, int32_t x, int32_t y) {
}

void d3dApp::onKeyDown(WPARAM btnState) {

}

void d3dApp::onKeyUp(WPARAM btnState) {

}

// 启用D3D12D的调试层
void d3dApp::enableDebugLayer() {
    ComPtr<ID3D12Debug> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
    debugController->EnableDebugLayer();
    DXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
}

void d3dApp::logOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format) {
    UINT count = 0;
    UINT flags = 0;

    // 以nullptr作为参数调用此函数来获取符合条件的显示模式的格式
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for (auto& mode : modeList) {
        UINT numerator = mode.RefreshRate.Numerator;
        UINT denominator = mode.RefreshRate.Denominator;

        std::wstring text = L"Width = " + std::to_wstring(mode.Width) + L" " +
        L"Height = " + std::to_wstring(mode.Height) + L" " +
        L"Refresh = " + std::to_wstring(numerator) + L"/" + std::to_wstring(denominator) + 
        L"\n";

        std::wcout << text;
    }
}

void d3dApp::logAdapterOutputs(IDXGIAdapter1* adapter) {
    UINT i = 0;
    IDXGIOutput* output = nullptr;

    while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND) {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        std::wcout << text;

        logOutputDisplayModes(output, backBufferFormat);
        ReleaseCom(output);

        i++;
    }
}

void d3dApp::calculateFrameStats() {
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.
    
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if( (timer.TotalTime() - timeElapsed) >= 1.0f )
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

        std::wstring fpsStr = std::to_wstring(fps);
        std::wstring mspfStr = std::to_wstring(mspf);

        std::wstring windowText = windowTitle +
            L"    fps: " + fpsStr +
            L"   mspf: " + mspfStr;

        SetWindowText(mainWindow, windowText.c_str());
		
		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void d3dApp::logAdapters(std::vector<IDXGIAdapter1*>& adapterList) {
    UINT i = 0; 
    IDXGIAdapter1* adapter = nullptr;
    
    while (DXGIFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        std::wcout << text;

        adapterList.push_back(adapter);

        i++;
    }

    for (size_t i = 0; i < adapterList.size(); i++) {
        logAdapterOutputs(adapterList[i]);
    }
}

void d3dApp::createDXGIFactory() {
    // 1.Create DXGIFactory
    // #define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)
    ThrowIfFailed(CreateDXGIFactory2(DXGIFactoryFlags, IID_PPV_ARGS(DXGIFactory.GetAddressOf())));
}

void d3dApp::createDevice() {
  #if defined(DEBUG) || defined(_DEBUG)
        enableDebugLayer();
    #endif

    std::vector<IDXGIAdapter1*> adapterList;

    // 2.Select Discrete video card(独立显卡)
    logAdapters(adapterList);

    // 对于只有单GPU的系统，适配器列表中的第一项就是
    // 第二项则是Microsoft Basic Render Driver
    IDXGIAdapter1* mainAdapter = adapterList[0];
    
    // 3.Create feature level 12.1 device
    // 第一个参数pAdapter可以直接传nullptr，会自动使用编号0的适配器
    ThrowIfFailed(D3D12CreateDevice(mainAdapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(device.GetAddressOf())));

    checkMSAASupport();
}

void d3dApp::checkMSAASupport() {
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
    
    qualityLevels.Format = backBufferFormat;
    qualityLevels.SampleCount = 8;
    qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    qualityLevels.NumQualityLevels = 0;

    ThrowIfFailed(device->CheckFeatureSupport(
    D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
    &qualityLevels,
    sizeof(qualityLevels)));
    
    MSAAQuality = qualityLevels.NumQualityLevels;
}

void d3dApp::createFence() {
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
}

void d3dApp::createCommandObjects() {
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};

    D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;

    queueDesc.Type = commandListType;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    // 4.Create command queue
    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(graphicsCommandQueue.GetAddressOf())));
    graphicsCommandQueue->SetName(L"graphicsCommandQueue");

    // 5.Crete command allocator
    ThrowIfFailed(device->CreateCommandAllocator(
    commandListType,
    IID_PPV_ARGS(commandAllocator.GetAddressOf())));

    // 6.Create command list
    ThrowIfFailed(device->CreateCommandList(
    0,
    commandListType,  
    commandAllocator.Get(),          // 关联命令分配器
    nullptr,                         // 初始化管线状态对象
    IID_PPV_ARGS(graphicsCommandList.GetAddressOf())));

    // 首先要将命令列表置于关闭状态。这是因为在第一次引用命令列表时，
    // 我们要对它进行重置，而在调用重置方法之前又需先将其关闭
    graphicsCommandList->Close();
}

void d3dApp::createSwapChain() {
    // 释放之前所创建的交换链，随后再进行重建
    swapChain.Reset();

    DXGI_SWAP_CHAIN_DESC swapChainDesc;

    swapChainDesc.BufferDesc.Width = windowWidth;
    swapChainDesc.BufferDesc.Height = windowHeight;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = backBufferFormat;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.SampleDesc.Count = MSAA4X ? 4 : 1;
    swapChainDesc.SampleDesc.Quality = MSAA4X ? (MSAAQuality - 1) : 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = frameBackBufferCount;
    swapChainDesc.OutputWindow = mainWindow;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // 7.Create swapchain
    // 注意，交换链需要通过命令队列对其进行刷新
    // 交换链创建好之后，包含frameBackBufferCount个缓冲区
    ThrowIfFailed(DXGIFactory->CreateSwapChain(
                graphicsCommandQueue.Get(),
                &swapChainDesc,
                temporarySwapChain.GetAddressOf()));

    // 使用WRL::ComPtr的As函数，其内部就是调用QueryInterface的经典COM方法
    // 使用低版本的ID3D12SwapChain接口获得高版本的ID3D12SwapChain4接口
    // 目的是为了调用GetCurrentBackBufferIndex方法，而从其来自高版本接口可以知道，
    // 这是后来扩展的方法。主要原因就是现在翻转绘制缓冲区到显示缓冲区的方法更高效了，
    // 直接就是将对应的后缓冲序号设置为当前显示的缓冲序号即可，比如原来显示的是序号为1的缓冲区，
    // 那么下一个要显示的缓冲区就是序号2的缓冲区，如果为2的缓冲区正在显示，那么下一个将要显示的序号就又回到了0
    ThrowIfFailed(temporarySwapChain.As(&swapChain));
}

void d3dApp::createRTVAndDSVDescriptorHeaps() {

    // 这里的几个Size后面会用于在描述符堆中偏移寻址访问各个View时用到
    // 比如创建RTV描述符堆的时候指定NumDescriptors = 3，则堆中就有3个RTV描述符
    // 
    RTVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    DSVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    CBVSRVUAVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC RTVDescriptorHeapDesc = {};

    RTVDescriptorHeapDesc.NumDescriptors = frameBackBufferCount;
    RTVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    RTVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    RTVDescriptorHeapDesc.NodeMask = 0;

    // 8.Create RTV descriptor heap
    ThrowIfFailed(device->CreateDescriptorHeap(
    &RTVDescriptorHeapDesc, IID_PPV_ARGS(RTVDescriptorHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC DSVDescriptorHeapDesc = {};

    DSVDescriptorHeapDesc.NumDescriptors = 1;
    DSVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    DSVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DSVDescriptorHeapDesc.NodeMask = 0;

    // 9.Create DSV descriptor heap
    ThrowIfFailed(device->CreateDescriptorHeap(
    &DSVDescriptorHeapDesc, IID_PPV_ARGS(DSVDescriptorHeap.GetAddressOf())));
}

ID3D12Resource* d3dApp::currentBackBuffer() const {
	return frameBackBuffers[currentFrameBackBufferIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE d3dApp::currentBackBufferView() {
    // CD3DX12构造函数根据给定的偏移量找到当前后台缓冲区的RTV
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),  // 堆中的首个句柄
        currentFrameBackBufferIndex,                    // 偏移至后台缓冲区描述符句柄的索引
        RTVDescriptorSize);                             // 描述符所占字节大小
}

D3D12_CPU_DESCRIPTOR_HANDLE d3dApp::depthStencilView() {
    return DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

void d3dApp::createRenderTargetViews() {

    for (UINT i = 0; i < frameBackBufferCount; i++) {
         // 获取描述符堆起始句柄(可以理解为数组的&array[0])
        // 而Offset函数也印证了描述符堆本质上的数组特性
        CD3DX12_CPU_DESCRIPTOR_HANDLE RTVDescriptorHeapHandle(
        RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        // 获得交换链内的第i个缓冲区
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(frameBackBuffers[i].GetAddressOf())));

        // 偏移到描述符堆中的下一个缓冲区
        RTVDescriptorHeapHandle.Offset(i, RTVDescriptorSize);

        // 9.Create render target views
        // 为此缓冲区创建一个关联的RTV描述符
        device->CreateRenderTargetView(
        frameBackBuffers[i].Get(), nullptr, RTVDescriptorHeapHandle);
    }
}

void d3dApp::createDepthStencilView() {
    // 创建深度/模板缓冲区及其视图
    D3D12_RESOURCE_DESC depthStencilDesc = {};

    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = windowWidth;
    depthStencilDesc.Height = windowHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = depthStencilBufferFormat;
    depthStencilDesc.SampleDesc.Count = MSAA4X ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = MSAA4X ? (MSAAQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = depthStencilBufferFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    // D3D12_HEAP_TYPE_DEFAULT表示资源直接创建在显存中
    // 因为深度缓冲不需要CPU访问
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &clearValue,
        IID_PPV_ARGS(depthStencilBuffer.GetAddressOf())));

    // 10.Create depth stencil view
    // 利用此资源格式，为整个资源的第0 mip层创建描述符
    device->CreateDepthStencilView(depthStencilBuffer.Get(),
    nullptr,
    depthStencilView());

    // 将资源从初始状态转换为深度缓冲区
    graphicsCommandList->ResourceBarrier(
        1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            depthStencilBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void d3dApp::setupViewport() {
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(windowWidth);
    viewport.Height = static_cast<float>(windowHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    graphicsCommandList->RSSetViewports(1, &viewport);

    scissorRect = {0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight)};
    graphicsCommandList->RSSetScissorRects(1, &scissorRect);
}

float d3dApp::aspectRatio() const {
	return static_cast<float>(windowWidth) / windowHeight;
}

int32_t d3dApp::run() {
    MSG msg = {0};
 
	timer.Reset();

    timer.Tick();

    float simulatedTime = 0.0f;

	while(msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		// Otherwise, do animation/game stuff.
		else
        {	
			timer.Tick();

            float realTime = timer.TotalTime();

            while (simulatedTime < realTime) {
                if (bFrameLimit) {
                    simulatedTime += 0.016667f;
                }

                calculateFrameStats();
				update(0.016667f);	
                render(0.016667f);
            }
        }
    }

	return (int)msg.wParam;
}
