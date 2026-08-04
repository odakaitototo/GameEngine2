#pragma once

#include "Engine/Component/ComponentBase.h"

// 当たり判定の種類
enum class ColliderType
{
	AABB,
	OBB
};

class ColliderBase : public ComponentBase
{
public:
	// コンストラクタで自分の種類を記憶させる
	ColliderBase(ColliderType type) : m_colliderType(type){}
	virtual ~ColliderBase() = default;

	// 自分の種類を返す関数
	ColliderType GetColliderType() const { return m_colliderType; }

	// 誰かにぶつかっているかを判定するフラグ
	bool isColliding = false;

protected:
	ColliderType m_colliderType;
};
