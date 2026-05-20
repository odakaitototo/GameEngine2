#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

//////////////----メモ------//////////////////////////////////////////////
//
// Device: メモリの確保やリソースを作る役割。　GameObjectやMeshのデータを作るのはこれ
// 
// Context: Deviceが作った道具を使って「描画せよ」とGPUに命令を出す役割。
//
//
//
//////////////////////////////////////////////////////////////////////////

#include "Engine/Graphics/DirectXManager.h"

DirectXManager::DirectXManager() {}
DirectXManager::~DirectXManager() {}

bool DirectXManager::Initialize(HWND hWnd, int width, int height) {
    // スワップチェーンの設定
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1; // 裏画面の数
    sd.BufferDesc.Width = width; // 描画する横幅
    sd.BufferDesc.Height = height; // 描画する縦幅
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // このバッファを描画先として使う設定
    sd.OutputWindow = hWnd; // どのウィンドウに表示するか
    sd.SampleDesc.Count = 1; // アンチエイリアス（ギザギザ補正）
    sd.Windowed = TRUE; // ウィンドウモードかフルスクリーンか

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    // デバッグ処理：デバッグ時のみDirectXの傾向機能をONにする。
    UINT creationFlags = 0;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG


    // DeviceとContextの作成
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, 
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 
        creationFlags,
        featureLevels, 1, // DirectX11を使うという宣言
        D3D11_SDK_VERSION,
        &sd, // 上で作った設定
        &m_pSwapChain, // [出力]作成されたスワップチェーン
        &m_pDevice, // [出力]作成されたデバイス
        &featureLevel,
        &m_pContext);  // [出力]作成されたコンテキスト

    if (FAILED(hr)) return false;

    // レンダーターゲット（描画先）の作成
    // スワップチェーンが持っている「裏画面テクスチャ」を、DirectXから操作できるように紐付けます。
    ComPtr<ID3D11Texture2D> pBackBuffer;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)); // 0番目のバッファ（裏画面）を取り出す
    m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pRenderTarget); // それを描画ターゲットとして登録

    // GPUに「ウィンドウのどの範囲を3D描画の領域にするか」を伝える
    
    m_viewport.Width = static_cast<float>(width); // 画面の横幅
    m_viewport.Height = static_cast<float>(height); // 画面の縦幅
    m_viewport.MinDepth = 0.0f; // 手前の奥行（0が最奥）
    m_viewport.MaxDepth = 1.0f; // 奥の奥行（1が最奥）
    m_viewport.TopLeftX = 0; // 左側の開始位置
    m_viewport.TopLeftY = 0; // 右側の開始位置

   
    // カリングをオフにする設定
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID; // 塗りつぶす
    rd.CullMode = D3D11_CULL_NONE; // 裏表関係なく描画する
    rd.FrontCounterClockwise = FALSE;
    m_pDevice->CreateRasterizerState(&rd, &m_pRasterizerState);


    return true;
}

void DirectXManager::BeginScene(float r, float g, float b, float a) {
    float color[4] = { r, g, b, a }; // 背景色（RGBA）
    m_pContext->ClearRenderTargetView(m_pRenderTarget.Get(), color); // 指定した色で画面を塗りつぶす（前のフレームの絵を消す）
    m_pContext->OMSetRenderTargets(1, m_pRenderTarget.GetAddressOf(), nullptr); // 「今からここに描画します」という宣言

    // 毎フレームビューポートを設定（ImGuiとGameシーンで描画の形式を書き換えあうから）
    m_pContext->RSSetViewports(1, &m_viewport);

    m_pContext->RSSetState(m_pRasterizerState.Get()); // 両面描画ルール

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff); // ブレンドステート（透明度）をリセット
    m_pContext->OMSetDepthStencilState(nullptr, 0); // 深度すてーとをリセット
}

void DirectXManager::EndScene() {
    // 裏画面と表画面を入れ替える(Present)
    // 第1引数を1にすると、ディスプレイのリフレッシュレートに合わせて表示（垂直同期）します。
    m_pSwapChain->Present(1, 0); // 垂直同期（1）で画面を更新
}