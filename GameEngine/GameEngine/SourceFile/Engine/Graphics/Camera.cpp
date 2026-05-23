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