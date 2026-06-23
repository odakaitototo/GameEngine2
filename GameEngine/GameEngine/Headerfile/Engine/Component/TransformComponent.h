#pragma once
#include "ComponentBase.h"
#include <DirectXMath.h>

// ComponentBaseを継承して、Transformの部品を作る
class TransformComponent : public ComponentBase
{
public:
	TransformComponent() = default;
	~TransformComponent() = default;

	// GameObjectの座標データ
	DirectX::XMFLOAT3 position = { 0.0f,0.0f,0.0f }; // 位置
	DirectX::XMFLOAT3 rotation = { 0.0f,0.0f,0.0f }; // 回転
	DirectX::XMFLOAT3 scale    = { 1.0f,1.0f,1.0f }; // 大きさ
};
