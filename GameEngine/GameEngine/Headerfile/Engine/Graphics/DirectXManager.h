#pragma once
#include <d3d11.h>
#include <wrl/client.h> // Microsoft::WRL::Comptrを使用するためのヘッダーファイル

using Microsoft::WRL::ComPtr;

class DirectXManager
{
public:
	DirectXManager();
	~DirectXManager();

	bool Initialize(HWND hwnd, int width, int height);
	void BeginScene(float r, float g, float b, float a); // 画面をクリア
	void EndScene(); // 画面を表示


	// ImGuiなどがDirectXの本体にアクセスするための窓口
	ID3D11Device* GetDevice() const { return m_pDevice.Get(); }
	ID3D11DeviceContext* GetContext() const { return m_pContext.Get(); }
	
private:
	ComPtr<ID3D11Device> m_pDevice; // 設備
	ComPtr<ID3D11DeviceContext> m_pContext; // 実行指示
	ComPtr<IDXGISwapChain> m_pSwapChain; // 画面の入れ替え
	ComPtr<ID3D11RenderTargetView> m_pRenderTarget; // 描画先
	D3D11_VIEWPORT m_viewport = {}; // ビューポートを記憶しておく変数
	ComPtr<ID3D11RasterizerState> m_pRasterizerState; // 描画のルールを保存する変数
	
private:// Zバッファ用の変数
	ComPtr<ID3D11Texture2D> m_pDepthStencilBuffer; // 奥行を記録するもの
	ComPtr<ID3D11DepthStencilView> m_pDepthStencilView; // 記録する際に使うもの
	ComPtr<ID3D11DepthStencilState> m_pDepthStencilState; // 手前の場合塗るというルール
};
