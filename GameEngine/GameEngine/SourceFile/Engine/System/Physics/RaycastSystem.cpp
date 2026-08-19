#include "Engine/System/Physics/RaycastSystem.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Component/ColliderBase.h"
#include "Engine/Component/AABBColliderComponent.h"
#include "Engine/Component/OBBColliderComponent.h"

RaycastHit RaycastSystem::Raycast(const Ray& ray, const std::vector<std::shared_ptr<GameObject>>& gameObject)
{
	RaycastHit result;
	result.distance = FLT_MAX; // 当たった距離の初期値は無限大

	// DirectXの計算用ベクトルに変換
	DirectX::XMVECTOR rayOrigin = DirectX::XMLoadFloat3(&ray.origin);
	DirectX::XMVECTOR rayDir = DirectX::XMLoadFloat3(&ray.direction);


	// レーザーの方向の長さを、正規化する
	rayDir = DirectX::XMVector3Normalize(rayDir);

	// 描画されているオブジェクトを1つずつ確認する
	for (const auto& obj : gameObject)
	{
		auto collider = obj->GetComponent<ColliderBase>();
		// オブジェクトがColliderを持っていなければ何もしない
		if (!collider)
		{
			continue;
		}

		float dist = 0.0f;
		bool hitThis = false;

		// AABBの場合
		if (collider->GetColliderType() == ColliderType::AABB)
		{
			auto aabb = std::static_pointer_cast<AABBColliderComponent>(collider);
			hitThis = aabb->worldBoundingBox.Intersects(rayOrigin, rayDir, dist);
		}
		// OBBの場合
		else if (collider->GetColliderType() == ColliderType::OBB)
		{
			auto obb = std::static_pointer_cast<OBBColliderComponent>(collider);
			hitThis = obb->worldBoundingBox.Intersects(rayOrigin, rayDir, dist);
		}

		// もし当たっていてかつ今まで見つけたオブジェクトよりも手前にあったら
		if (hitThis && dist < result.distance)
		{
			// resultの記録を更新する
			result.isHit = true;
			result.distance = dist;
			result.hitObject = obj;
		}
	}

	return result;
}