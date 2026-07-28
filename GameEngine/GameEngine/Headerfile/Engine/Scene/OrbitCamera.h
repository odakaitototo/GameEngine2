#pragma once
#include <DirectXMath.h>

class OrbitCamera
{
public:
	OrbitCamera();

	// マイフレーム呼びだして、カメラの座標を更新する
	void Update();

	// マウスやコントローラーの入力でカメラを回す
	void Rotate(float deltaYaw, float deltaPitch);

	// ズームイン・ズームアウト
	void Zoom(float deltaDistance);

	// ターゲットの変更
	void SetTarget(const DirectX::XMFLOAT3& target) { m_target = target; }

	// DirectXに渡すための行列を取得
	DirectX::XMMATRIX GetViewMatrix() const;
	DirectX::XMMATRIX GetProjectionMatrix() const;

	// 画面サイズが変った時にアスペクト比を調整する
	void SetPerspective(float fovDeg, float aspect, float nearZ, float farZ);

private:
	DirectX::XMFLOAT3 m_target; // 見つめる中心点
	float m_distance; // ターゲットからの距離
	float m_yaw; // 水平回転
	float m_pitch; // 上下角度

	DirectX::XMFLOAT4X4 m_viewMatrix;
	DirectX::XMFLOAT4X4 m_projMatrix;
};