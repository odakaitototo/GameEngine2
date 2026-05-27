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

public: // ビューボート化
	bool CreateSceneResources(int width, int height); // ビューポートの作成

	void BeginSceneTexture(float width, float height, float r, float g, float b, float a);

	ID3D11RenderTargetView* GetSceneRTV() const { return m_pSceneRTV.Get(); }
	ID3D11ShaderResourceView* GetSceneSRV() const { return m_pSceneSRV.Get(); }
	ID3D11DepthStencilView* GetSceneDSV() const { return m_pSceneDSV.Get(); }

	
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


	
private: // ビューボート化

	ComPtr<ID3D11Texture2D> m_pSceneColorTexture; // カラー用のテクスチャ実体
	ComPtr<ID3D11RenderTargetView> m_pSceneRTV; // 描画先として指定するためのRTV
	ComPtr<ID3D11ShaderResourceView> m_pSceneSRV; // ImGuiに描画として渡すためのSRV
	ComPtr<ID3D11Texture2D> m_pSceneDepthTexture; // ImGuiに画像として渡すためのSRV

	ComPtr<ID3D11DepthStencilView> m_pSceneDSV; // 3Dの前後関係を正しく計算するためのDSV


};
