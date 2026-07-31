#include "Engine//Component/OBBColliderComponent.h"
#include "Engine/Component/TransformComponent.h"
#include "Engine/Scene/GameObject.h"

OBBColliderComponent::OBBColliderComponent() : ColliderBase(ColliderType::OBB)
{
	localBoundingBox = DirectX::BoundingOrientedBox(
		DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f),
		DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f,1.0f) // 回転情報
	);
}

void OBBColliderComponent::LateUpdate()
{
	auto transform = gameObject->GetComponent<TransformComponent>();
	if (transform != nullptr)
	{
		// 親の最新のワールド行列を取得
		DirectX::XMMATRIX worldMat = DirectX::XMLoadFloat4x4(&transform->worldMatrix);
		localBoundingBox.Transform(worldBoundingBox, worldMat);
	}
}