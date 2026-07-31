#pragma once
#include "Engine/Component/ColliderBase.h"
#include <DirectXcollision.h>

class OBBColliderComponent : public ColliderBase
{
public:
	OBBColliderComponent();
	virtual ~OBBColliderComponent() = default;

	virtual void LateUpdate() override;

	DirectX::BoundingOrientedBox localBoundingBox;
	DirectX::BoundingOrientedBox worldBoundingBox;

};