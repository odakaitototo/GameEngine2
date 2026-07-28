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

public: // Editorでカメラの数値をいじれるようにするための関数

	// ImGuiから直接数値を返せるようにするために、参照(&)で返す
	DirectX::XMFLOAT3& GetTarget() { return m_target; }
	float& GetDistance() { return m_distance; }
	float& GetYaw() { return m_yaw; }
	float& GetPitch() { return m_pitch; }

	// 限界地とスピード設定のアクセス用
	float& GetMinPitch() { return m_minPitch; }
	float& GetMaxPitch() { return  m_maxPitch; }
	float& GetMinDistance() { return m_minDistance; }
	float& GetMaxDistance() { return m_maxDistance; }
	float& GetRotateSpeed() { return m_rotateSpeed; }
	float& GetZoomSpeed() { return m_zoomSpeed; }

private:
	DirectX::XMFLOAT3 m_target; // 見つめる中心点
	float m_distance; // ターゲットからの距離
	float m_yaw; // 水平回転
	float m_pitch; // 上下角度

	DirectX::XMFLOAT4X4 m_viewMatrix;
	DirectX::XMFLOAT4X4 m_projMatrix;

private: // Editorでカメラの数値をいじれるようにするための変数
	float m_minPitch = 8.0f; // 下を向く限界値の初期値
	float m_maxPitch = 80.0f; // 上を向く限界値の初期値
	float m_minDistance = 2.0f; // ズームインの限界値の初期値
	float m_maxDistance = 50.0f; // ズームアウトの限界値の初期値
	float m_rotateSpeed = 0.3f; // 回転スピードの初期値
	float m_zoomSpeed = 1.5f; // ズームイン・アウトのスピードの初期値
};