#pragma once

#include "ComponentBase.h"
#include "ColliderBase.h"
#include <DirectXCollision.h>

// ComponentBaseを継承して、当たり判定の部品を作る
class AABBColliderComponent : public ColliderBase
{
public:
	AABBColliderComponent();
	
	virtual ~AABBColliderComponent() = default;

	// 毎フレーム呼ばれる更新処理
	virtual void LateUpdate() override;

	DirectX::BoundingBox localBoundingBox; // 基準となる大きいさ
	DirectX::BoundingBox worldBoundingBox; // 実際にゲーム世界で当たり判定に使う箱

};