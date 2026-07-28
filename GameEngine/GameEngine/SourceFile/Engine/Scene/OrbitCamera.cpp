#include "Engine/Scene/OrbitCamera.h"
#include <algorithm>

OrbitCamera::OrbitCamera()
	:m_target(0.0f, 0.0f, 0.0f)
	, m_distance(15.0f) // 初期状態はターゲットから15メートル離れた状態
	, m_yaw(45.0f) //　斜め45度から見下ろす
	,m_pitch(30.0f)
{
	// 初期行列の作成
	SetPerspective(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
	Update();
}

void OrbitCamera::Update()
{
	// DirectXに渡すために一次的に度数法からラジアンに変更する
	float radYaw   = DirectX::XMConvertToRadians(m_yaw);
	float radPitch = DirectX::XMConvertToRadians(m_pitch);

	// ターゲットから「(0, 0, -距離)」だけ後ろに退いた位置を、指定の角度で回転させる！
	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(radPitch, radYaw, 0.0f);
	DirectX::XMVECTOR offset = DirectX::XMVectorSet(0.0f, 0.0f, -m_distance, 0.0f);
	offset = DirectX::XMVector3TransformNormal(offset, rotation);

	// カメラの現在位置 = ターゲット位置 + 回転後のオフセット
	DirectX::XMVECTOR targetPos = DirectX::XMLoadFloat3(&m_target);
	DirectX::XMVECTOR cameraPos = DirectX::XMVectorAdd(targetPos, offset);

	// 「カメラ位置」から「ターゲット」を見つめる View行列(レンズ)を作成
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(cameraPos, targetPos, up);

	DirectX::XMStoreFloat4x4(&m_viewMatrix, view);
}

void OrbitCamera::Rotate(float deltaYaw, float deltaPitch)
{
	m_yaw += deltaYaw * m_rotateSpeed;
	m_pitch += deltaPitch * m_rotateSpeed;

	// 上下の回転に制限をかける
	m_pitch = std::clamp(m_pitch, m_minPitch, m_maxPitch);
}

void OrbitCamera::Zoom(float deltaDistance)
{
	m_distance -= deltaDistance * m_zoomSpeed;
	m_distance = std::clamp(m_distance, m_minDistance, m_maxDistance); // 前後にも制限を書ける
}

void OrbitCamera::SetPerspective(float fovDeg, float aspect, float nearZ, float farZ)
{
	float fovRad = DirectX::XMConvertToRadians(fovDeg);
	DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(fovRad, aspect, nearZ, farZ);
	DirectX::XMStoreFloat4x4(&m_projMatrix, proj);
}

DirectX::XMMATRIX OrbitCamera::GetViewMatrix() const
{
	return DirectX::XMLoadFloat4x4(&m_viewMatrix);
}

DirectX::XMMATRIX OrbitCamera::GetProjectionMatrix() const
{
	return DirectX::XMLoadFloat4x4(&m_projMatrix);
}