#include "Engine/Graphics/Camera.h"

Camera::Camera()
{
	// 初期値は単位行列（何も変化させない行列）にしておく
	m_viewMatrix = DirectX::XMMatrixIdentity();
	m_projectionMatrix = DirectX::XMMatrixIdentity();
}

Camera::~Camera()
{

}

void Camera::SetLookAt(DirectX::XMVECTOR position, DirectX::XMVECTOR target, DirectX::XMVECTOR up)
{
	m_viewMatrix = DirectX::XMMatrixLookAtLH(position, target, up);
}

void Camera::SetPerspective(float fovAngle, float aspectRatio, float nearZ, float farZ)
{
	m_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fovAngle, aspectRatio, nearZ, farZ);
}

void Camera::SetAspect(float width, float height)
{
	// 高さが0以下になるとと割り算エラーになるので
	// 0以下だとreturnする
	if (height <= 0.0f)
	{
		return;
	}

	float aspectRatio = width / height; // 縦横比を計算する

	SetPerspective(DirectX::XMConvertToRadians(45.0f), aspectRatio, 0.1f, 1000.0f);
}

void Camera::Update()
{
	// 横回転（ピッチ）の制限　（真上や真下を向きすぎて首が折れないようにする）
	if (m_pitch > 89.0f)
	{
		m_pitch = 89.0f;
	}
	if (m_pitch < -89.0f)
	{
		m_pitch = -89.0f;
	}

	// 角度をラジアンに変換して、回転行列を作る
	float pitchRad = DirectX::XMConvertToRadians(m_pitch);
	float yawRad = DirectX::XMConvertToRadians(m_yaw);
	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(pitchRad, yawRad, 0.0f);

	// カメラの基本方向き
	DirectX::XMVECTOR forward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// 回転行列を掛けて、ベクトルを計算する
	DirectX::XMVECTOR lookVector = DirectX::XMVector3TransformNormal(forward, rotationMatrix);
	DirectX::XMVECTOR upVector = DirectX::XMVector3TransformNormal(up, rotationMatrix);

	// カメラの位置
	DirectX::XMVECTOR posVector = DirectX::XMLoadFloat3(&m_position);

	// 見つめるターゲット = カメラの位置 + 今見ている方向
	DirectX::XMVECTOR target = DirectX::XMVectorAdd(posVector, lookVector);

	// ビュー行列
	m_viewMatrix = DirectX::XMMatrixLookAtLH(posVector, target, upVector);

}

void Camera::Move(float dRight, float dUp, float dForward)
{
	// カメラの向いている方向を基準にして、前後左右の「移動ベクトル」を計算する
	float pitchRad = DirectX::XMConvertToRadians(m_pitch);
	float yawRad = DirectX::XMConvertToRadians(m_yaw);
	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(pitchRad, yawRad, 0.0f);

	DirectX::XMVECTOR forward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	DirectX::XMVECTOR right   = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMVECTOR lookVector  = DirectX::XMVector3TransformNormal(forward, rotationMatrix);
	DirectX::XMVECTOR rightVector = DirectX::XMVector3TransformNormal(right, rotationMatrix);
	DirectX::XMVECTOR upVector    = DirectX::XMVector3TransformNormal(up, rotationMatrix);

	DirectX::XMVECTOR posVector = DirectX::XMLoadFloat3(&m_position);

	// 現在の位置に、それぞれの移動量を足し合わせる
	posVector = DirectX::XMVectorAdd(posVector, DirectX::XMVectorScale(rightVector, dRight));
	posVector = DirectX::XMVectorAdd(posVector, DirectX::XMVectorScale(upVector, dUp));
	posVector = DirectX::XMVectorAdd(posVector, DirectX::XMVectorScale(lookVector, dForward));

	DirectX::XMStoreFloat3(&m_position, posVector);

}

void Camera::Rotate(float dPitch, float dYaw)
{
	m_pitch += dPitch; // 縦の回転を加算
	m_yaw += dYaw; // 横の回転を加算
}