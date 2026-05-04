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

	
};
