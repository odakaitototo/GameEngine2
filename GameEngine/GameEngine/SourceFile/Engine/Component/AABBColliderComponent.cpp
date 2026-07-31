#include "Engine/Component/AABBColliderComponent.h"
#include "Engine/Scene/GameObject.h" // gameObject -> GetComponent を使うために必要
#include "Engine/Component/TransformComponent.h" // Transform

AABBColliderComponent::AABBColliderComponent() : ColliderBase(ColliderType::AABB)
{
	// 基準となる1 * 1 * 1の箱を初期化
	localBoundingBox = DirectX::BoundingBox(
		DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f)
	);
}

void AABBColliderComponent::LateUpdate()
{
	// 自分がくっついてるGameObjectが存在するかチェック
	if (gameObject != nullptr)
	{
		// 親のGameObjectから、Transform部品をもらってくる
		auto transform = gameObject->GetComponent<TransformComponent>();
		if (transform != nullptr)
		{
			// 親のTransformの座標を入れる
			DirectX::XMMATRIX worldMat = DirectX::XMLoadFloat4x4(&transform->worldMatrix);


			localBoundingBox.Transform(worldBoundingBox, worldMat);
		}
	}
}