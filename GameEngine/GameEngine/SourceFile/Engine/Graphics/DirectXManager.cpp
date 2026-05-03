#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include "Engine/Graphics/DirectXManager.h"

DirectXManager::DirectXManager() {}
DirectXManager::~DirectXManager() {}

bool DirectXManager::Initialize(HWND hWnd, int width, int height) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    // デバイスとスワップチェーンの作成
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, 1,
        D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pDevice, &featureLevel, &m_pContext);

    if (FAILED(hr)) return false;

    // レンダーターゲット（描画先）の作成
    ComPtr<ID3D11Texture2D> pBackBuffer;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pRenderTarget);

    return true;
}

void DirectXManager::BeginScene(float r, float g, float b, float a) {
    float color[4] = { r, g, b, a };
    m_pContext->ClearRenderTargetView(m_pRenderTarget.Get(), color);
    m_pContext->OMSetRenderTargets(1, m_pRenderTarget.GetAddressOf(), nullptr);
}

void DirectXManager::EndScene() {
    m_pSwapChain->Present(1, 0); // 垂直同期（1）で画面を更新
}