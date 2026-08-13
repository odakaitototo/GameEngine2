#pragma once
#include <DirectXMath.h>
#include "Engine/Component/ComponentBase.h"

class RigidbodyComponent :  public ComponentBase
{
public:
	bool useGravity = false; // 重力を使うか
	bool freezePosX = false; // X軸の移動を固定するか
	bool freezePosY = false; // Y軸の移動を固定するか
	bool freezePosZ = false; // Z軸の移動を固定するか

	DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f }; // オブジェクトのXYZ方向の移動速度の初期値

	// エディタから調節できるパラメータ
	float gravityScale = 1.0f; // 重力の強さ(1.0が通常0が無重力、マイナスなら上に落ちる)
	float drag = 0.0f; // 空気抵抗(0.0で抵抗なし、値が大きいほど抵抗力が強くなる)

	RigidbodyComponent() = default;
	~RigidbodyComponent() = default;
};