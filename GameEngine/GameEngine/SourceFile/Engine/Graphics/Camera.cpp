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