#pragma once 

#include "d3dApp.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"

#include <windef.h>

#include "imgui/imgui.h"

#include <dxcapi.h>
#include <wrl/client.h>

using namespace DirectX;

class shapesApp : public d3dApp {
public:

    shapesApp(HINSTANCE hInstance, const uint32_t inWindowWidth, const uint32_t inWindowHeight);
    ~shapesApp();

    virtual LRESULT msgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

    virtual bool initialize() override;

protected: 

	virtual void onResize() override; 
	virtual void update(float delta)  override;
    virtual void draw(float delta)  override;

	// Convenience overrides for handling mouse input.
	virtual void onMouseDown(WPARAM btnState, int32_t x, int32_t y) override;
	virtual void onMouseUp(WPARAM btnState, int32_t x, int32_t y) override;
	virtual void onMouseMove(WPARAM btnState, int32_t x, int32_t y) override;

    virtual void onKeyDown(WPARAM btnState) override;
    virtual void onKeyUp(WPARAM btnState) override;

    void createCBVSRVDescriptorHeaps();
    void createFrameResources();
    void createConstantBufferViews();
    void createShaderResourceView();
    void createSampler();
    void createRootSignature();
    void createShadersAndInputLayout();
    void createBoxGeometry();
    void createPipelineStateOjbect();

    void loadResources();

    void buildShapeGeometry();
    void buildRenderItems();

    void updateObjectConstantBuffers();
    void updatePassConstantBuffers();

    void resetCommandList();
    void executeCommandList();

    void prepareFrameResourceSync();
    void frameResourceSync();

    void initImGUI(); 
	void buildImGuiWidgets();
	void updateImGui();
	void renderImGui();
    void drawRenderItems(const std::vector<FrameUtil::RenderItem*>& renderItems);

private:

    ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> CBVDescriptorHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> SRVDescriptorHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> samplerDescriptorHeap = nullptr;
    
    std::unique_ptr<MeshGeometry> boxGeometry = nullptr;

    // ComPtr<ID3DBlob> vertexShaderByteCode = nullptr;
    // ComPtr<ID3DBlob> pixelShaderByteCode = nullptr;

    ComPtr<IDxcBlob> vertexShaderByteCode = nullptr;
    ComPtr<IDxcBlob> pixelShaderByteCode = nullptr;

    ComPtr<ID3D12Resource> texture;
    std::vector<ComPtr<ID3D12Resource>> textures;
    ComPtr<ID3D12Resource> textureUpload;

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;

    const uint32_t frameResourcesCount = 3;
    uint32_t currentFrameIndex = 0;

    std::vector<std::unique_ptr<FrameUtil::FrameResources>> frameResources;

    std::vector<std::unique_ptr<FrameUtil::RenderItem>> allRenderItems;

    FrameUtil::FrameResources* currentFrameResource;

    // 根据PSO来划分渲染项
    std::vector<FrameUtil::RenderItem*> opaqueRenderItems;
    std::vector<FrameUtil::RenderItem*> transparentRenderItems;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> geometries;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> graphicsPSOs;
    ComPtr<ID3D12PipelineState> computePSO;

    uint32_t objectCount = 1;

    XMFLOAT4X4 view = MathHelper::Identity4x4();
    XMFLOAT4X4 projection = MathHelper::Identity4x4();

    float theta = 1.5f * XM_PI;
    float phi = XM_PIDIV4;
    float radius = 5.0f;

    POINT lastMousePosition;

    bool isWireframe = false;
    	// Our state
	bool showMainWindow = true;
	bool showDemoWindow = true;
	bool showAnotherWindow = false;
	ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    DXGI_FORMAT textureFormat;
};