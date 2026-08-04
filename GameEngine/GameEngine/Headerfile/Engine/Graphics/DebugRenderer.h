#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>

class Application;

class DebugRenderer
{
public:
	DebugRenderer() = default;
	~DebugRenderer() = default;

	// 当たァり判定の箱を描画するメイン関数
	void DrawColliders(Application* app, DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix);

private:
	// 最初の一度だけ設定を作る関数
	void InitializeIfNeeded(ID3D11Device* device);

	// DirectXの描画状態を記憶しておく変数
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_wireFrameRS;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_solidRS;

	bool m_isInitialized = false; // 初期化済みかどうかのフラグ
};