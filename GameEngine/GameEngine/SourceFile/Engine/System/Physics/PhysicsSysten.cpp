#include "Engine/System/Physics/PhysicsSystem.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Component/ColliderBase.h"
#include "Engine/Component/AABBColliderComponent.h"
#include "Engine/Component/OBBColliderComponent.h"
#include "Engine/Component/RigidbodyComponent.h"

static bool CalculateOBBPenetration(const DirectX::BoundingOrientedBox& obbA, const DirectX::BoundingOrientedBox& obbB, DirectX::XMFLOAT3& outMTV)
{
	using namespace DirectX;

	XMVECTOR centerA = XMLoadFloat3(&obbA.Center);
	XMVECTOR centerB = XMLoadFloat3(&obbB.Center);
	XMVECTOR quatA = XMLoadFloat4(&obbA.Orientation);
	XMVECTOR quatB = XMLoadFloat4(&obbB.Orientation);

	// 箱Aと箱Bの「ローカル軸（向いている方向）」を計算
	XMMATRIX matA = XMMatrixRotationQuaternion(quatA);
	XMMATRIX matB = XMMatrixRotationQuaternion(quatB);
	XMVECTOR axesA[3] = { matA.r[0], matA.r[1], matA.r[2] };
	XMVECTOR axesB[3] = { matB.r[0], matB.r[1], matB.r[2] };

	float extA[3] = { obbA.Extents.x, obbA.Extents.y, obbA.Extents.z };
	float extB[3] = { obbB.Extents.x, obbB.Extents.y, obbB.Extents.z };

	XMVECTOR testAxes[15];
	int numAxes = 0;

	for (int i = 0; i < 3; ++i) testAxes[numAxes++] = axesA[i]; // Aの3軸
	for (int i = 0; i < 3; ++i) testAxes[numAxes++] = axesB[i]; // Bの3軸

	// AとBの軸の外積（9軸）
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			XMVECTOR cross = XMVector3Cross(axesA[i], axesB[j]);
			// 軸同士が平行な場合はゼロベクトルになるので計算を省く
			if (XMVectorGetX(XMVector3LengthSq(cross)) > 1e-5f) {
				testAxes[numAxes++] = XMVector3Normalize(cross);
			}
		}
	}

	// 15個の軸すべてで「影の重なり」をチェック
	float minOverlap = FLT_MAX;
	XMVECTOR mtvAxis = XMVectorZero();
	XMVECTOR d = XMVectorSubtract(centerB, centerA); // AからBへのベクトル

	for (int i = 0; i < numAxes; ++i) {
		XMVECTOR axis = XMVector3Normalize(testAxes[i]);

		// 箱Aを軸に投影した時の「影の半径」
		float rA = 0;
		for (int j = 0; j < 3; ++j) rA += extA[j] * std::abs(XMVectorGetX(XMVector3Dot(axesA[j], axis)));

		// 箱Bを軸に投影した時の「影の半径」
		float rB = 0;
		for (int j = 0; j < 3; ++j) rB += extB[j] * std::abs(XMVectorGetX(XMVector3Dot(axesB[j], axis)));

		// 2つの箱の中心の距離
		float dist = std::abs(XMVectorGetX(XMVector3Dot(d, axis)));

		// 重なり具合を計算
		float overlap = rA + rB - dist;

		// 1つでも重なっていない軸があれば、衝突していない
		if (overlap <= 0.0f) {
			return false;
		}

		// 一番重なりが浅い軸を記録する
		if (overlap < minOverlap) {
			minOverlap = overlap;
			mtvAxis = axis;
		}
	}

	// 押し出す方向がAからBへ向くように調整
	if (XMVectorGetX(XMVector3Dot(d, mtvAxis)) < 0) {
		mtvAxis = XMVectorNegate(mtvAxis); // 逆向きなら反転
	}

	// 最終的な押し出しベクトル（方向 × めり込み量）を保存
	XMStoreFloat3(&outMTV, XMVectorScale(mtvAxis, minOverlap));
	return true; // 衝突している
	
}


void PhysicsSystem::Update(const std::vector<std::shared_ptr<GameObject>>& gameObjects)
{
	// まず全員の「ぶつかっているフラグ」をリセットする
	for (auto& obj : gameObjects)
	{
		auto col = obj->GetComponent<ColliderBase>();
		if (col) col->isColliding = false;
	}

	// 衝突の計算
	for (size_t i = 0; i < gameObjects.size(); i++)
	{
		auto colA = gameObjects[i]->GetComponent<ColliderBase>();
		if (!colA)
		{
			continue;
		}

		for (size_t j = i + 1; j < gameObjects.size(); j++)
		{
			auto colB = gameObjects[j]->GetComponent<ColliderBase>();
			if (!colB)
			{
				continue;
			}

			bool hit = false;

			// 形の組み合わせによって計算式を振り分ける
			// AABB & AABB の場合
			if (colA->GetColliderType() == ColliderType::AABB && colB->GetColliderType() == ColliderType::AABB)
			{
				auto aabbA = std::static_pointer_cast<AABBColliderComponent>(colA);
				auto aabbB = std::static_pointer_cast<AABBColliderComponent>(colB);
				
				//ぶつかっている場合
				if (aabbA->worldBoundingBox.Intersects(aabbB->worldBoundingBox))
				{
					hit = true; // ぶつかっているかどうかのフラグをオン

					auto& boxA = aabbA->worldBoundingBox;
					auto& boxB = aabbB->worldBoundingBox;

					// boxAとboxBの中心距離を求める
					float dx = boxB.Center.x - boxA.Center.x;
					float dy = boxB.Center.y - boxA.Center.y;
					float dz = boxB.Center.z - boxA.Center.z;

					// サイズの合計
					float ex = boxA.Extents.x + boxB.Extents.x;
					float ey = boxA.Extents.y + boxB.Extents.y;
					float ez = boxA.Extents.z + boxB.Extents.z;

					// 各軸ごとにどれだけめり込んでいるか計算する
					float overlapX = ex - std::abs(dx);
					float overlapY = ey - std::abs(dy);
					float overlapZ = ez - std::abs(dz);

					
					

					// Rigidbodyを持っているかを見る
					// 持っていなかったらnullptr
					auto rigidbodyObjectA = gameObjects[i]->GetComponent<RigidbodyComponent>();
					auto rigidbodyObjectB = gameObjects[j]->GetComponent<RigidbodyComponent>();

					// weight（動く割合）1.0なら動く、0.0なら動かない
					float weightA = (rigidbodyObjectA != nullptr) ? 1.0f : 0.0f; // 変数宣言と同時に書くif文： ? = trueの時  : = falseの時
					float weightB = (rigidbodyObjectB != nullptr) ? 1.0f : 0.0f;

					// もし両方 rigidbodyを持っていたら半分ずつ押し出すことで反発させる
					if (rigidbodyObjectA != nullptr && rigidbodyObjectB != nullptr)	
					{
						weightA = 0.5f;
						weightB = 0.5f;
					}

					// 両方TRigidbodyを持っていなかったらBを動かす
					if (rigidbodyObjectA == nullptr && rigidbodyObjectB == nullptr)
					{
						weightB = 1.0f;
					}


					
					auto& tA = gameObjects[i]->GetTransform();
					auto& tB = gameObjects[j]->GetTransform();



					// 一番めり込みが浅い方向を探して押し出す
					if (overlapX <= overlapY && overlapX <= overlapZ)
					{
						// 押し出すことでめり込まないようにする
						float pushX = (dx > 0) ? overlapX : -overlapX;
						tA.position.x -= pushX * weightA;
						tB.position.x += pushX * weightB;


						if (rigidbodyObjectA != nullptr)
						{
							rigidbodyObjectA->velocity.x = 0.0f;
						}

						if (rigidbodyObjectB != nullptr)
						{
							rigidbodyObjectB->velocity.x = 0.0f;
						}
					}
					else if (overlapY <= overlapX && overlapY <= overlapZ)
					{
						float pushY = (dy > 0) ? overlapY : -overlapY;
						tA.position.y -= pushY * weightA;
						tB.position.y += pushY * weightB;

						if (rigidbodyObjectA != nullptr)
						{
							rigidbodyObjectA->velocity.y = 0.0f;
						}

						if (rigidbodyObjectB != nullptr)
						{
							rigidbodyObjectB->velocity.y = 0.0f;
						}

					}
					else if (overlapZ <= overlapX && overlapZ <= overlapY)
					{
						float pushZ = (dy > 0) ? overlapZ : -overlapZ;
						tA.position.z -= pushZ * weightA;
						tB.position.z += pushZ * weightB;

						if (rigidbodyObjectA != nullptr)
						{
							rigidbodyObjectA->velocity.z = 0.0f;
						}

						if (rigidbodyObjectB != nullptr)
						{
							rigidbodyObjectB->velocity.z = 0.0f;
						}
					}

					// 動かした時だけTransform更新して最新状態にする
					if (weightA > 0.0f)
					{
						gameObjects[i]->UpdateTransform();
					}

					if (weightB > 0.0f)
					{
						gameObjects[j]->UpdateTransform();
					}

				}
			}
			// OBB同士の当たり判定
			else if (colA->GetColliderType() == ColliderType::OBB && colB->GetColliderType() == ColliderType::OBB)
			{
				auto obbA = std::static_pointer_cast<OBBColliderComponent>(colA);
				auto obbB = std::static_pointer_cast<OBBColliderComponent>(colB);

				DirectX::XMFLOAT3 mtv = { 0.0f, 0.0f, 0.0f };

				if (CalculateOBBPenetration(obbA->worldBoundingBox, obbB->worldBoundingBox, mtv))
				{
					hit = true; // ぶつかっている

					// RigidBodyがついてるかどうか　ついていなかったらnullptr
					auto rigidbodyObjectA = gameObjects[i]->GetComponent<RigidbodyComponent>();
					auto rigidbodyObjectB = gameObjects[j]->GetComponent<RigidbodyComponent>();

					// weight（動く割合）1.0なら動く、0.0なら動かない 
					float weightA = (rigidbodyObjectA != nullptr) ? 1.0f : 0.0f;
					float weightB = (rigidbodyObjectB != nullptr) ? 1.0f : 0.0f;

					if (rigidbodyObjectA != nullptr && rigidbodyObjectB != nullptr)
					{
						weightA = 0.5f;
						weightB = 0.5f;
					}

					if (rigidbodyObjectA == nullptr && rigidbodyObjectB == nullptr)
					{
						
						weightB = 1.0f;
					}

					// Transformの取得
					auto& tA = gameObjects[i]->GetTransform();
					auto& tB = gameObjects[j]->GetTransform();

					// OBBの複雑な回転を考慮した上で、正しい方向へ押し出す！
					tA.position.x -= mtv.x * weightA;
					tA.position.y -= mtv.y * weightA;
					tA.position.z -= mtv.z * weightA;

					tB.position.x += mtv.x * weightB;
					tB.position.y += mtv.y * weightB;
					tB.position.z += mtv.z * weightB;

					// 押し出された方向のスピードを0にする
					if (std::abs(mtv.x) > 0.0001f) 
					{
						if (rigidbodyObjectA != nullptr)
						{
							rigidbodyObjectA->velocity.x = 0.0f;
						}

						if (rigidbodyObjectB != nullptr) 
						{
							rigidbodyObjectB->velocity.x = 0.0f;
						}
					}

					if (std::abs(mtv.y) > 0.0001f) 
					{
						if (rigidbodyObjectA != nullptr)
						{
							rigidbodyObjectA->velocity.y = 0.0f;
						}

						if (rigidbodyObjectB != nullptr)
						{
							rigidbodyObjectB->velocity.y = 0.0f;
						}
					}

					if (std::abs(mtv.z) > 0.0001f) 
					{
						if (rigidbodyObjectA != nullptr)
						{
							rigidbodyObjectA->velocity.z = 0.0f;
						}

						if (rigidbodyObjectB != nullptr)
						{
							rigidbodyObjectB->velocity.z = 0.0f;
						}
					}

					// 動いたオブジェクトだけ行列を更新
					if (weightA > 0.0f)
					{
						gameObjects[i]->UpdateTransform();
					}

					if (weightB > 0.0f)
					{
						gameObjects[j]->UpdateTransform();
					}
				}
			}
			// AABB と OBB または OBB と ABB の場合
			else if ((colA->GetColliderType() == ColliderType::AABB && colB->GetColliderType() == ColliderType::OBB) ||
				(colA->GetColliderType() == ColliderType::OBB && colB->GetColliderType() == ColliderType::AABB))
			{
				DirectX::BoundingOrientedBox obbA, obbB;
				// AABBは回転していないOBBと同じ扱いができるので

				// 1. Aの箱がAABBだった場合OBBの形式に合わせる
				if (colA->GetColliderType() == ColliderType::AABB) {
					auto aabb = std::static_pointer_cast<AABBColliderComponent>(colA);
					obbA.Center = aabb->worldBoundingBox.Center;
					obbA.Extents = aabb->worldBoundingBox.Extents;
					obbA.Orientation = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); // 無回転
				}
				else {
					obbA = std::static_pointer_cast<OBBColliderComponent>(colA)->worldBoundingBox;
				}

				// 2. Bの箱がAABBだった場合OBBの形式に合わせる
				if (colB->GetColliderType() == ColliderType::AABB) {
					auto aabb = std::static_pointer_cast<AABBColliderComponent>(colB);
					obbB.Center = aabb->worldBoundingBox.Center;
					obbB.Extents = aabb->worldBoundingBox.Extents;
					obbB.Orientation = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); // 無回転
				}
				else {
					obbB = std::static_pointer_cast<OBBColliderComponent>(colB)->worldBoundingBox;
				}

				DirectX::XMFLOAT3 mtv = { 0.0f, 0.0f, 0.0f };

				// 両方ともOBBの形にした後にOBB用の計算式を使い当たり判定を計算する
				if (CalculateOBBPenetration(obbA, obbB, mtv))
				{
					hit = true; // ぶつかっている

					// RigidBodyがついてるかどうか　ついていなかったらnullptr
					auto rigidbodyObjectA = gameObjects[i]->GetComponent<RigidbodyComponent>();
					auto rigidbodyObjectB = gameObjects[j]->GetComponent<RigidbodyComponent>();

					// weight（動く割合）1.0なら動く、0.0なら動かない 
					float weightA = (rigidbodyObjectA != nullptr) ? 1.0f : 0.0f;
					float weightB = (rigidbodyObjectB != nullptr) ? 1.0f : 0.0f;

					if (rigidbodyObjectA != nullptr && rigidbodyObjectB != nullptr)
					{
						weightA = 0.5f;
						weightB = 0.5f;
					}

					if (rigidbodyObjectA == nullptr && rigidbodyObjectB == nullptr)
					{

						weightB = 1.0f;
					}

					// Transformの取得
					auto& tA = gameObjects[i]->GetTransform();
					auto& tB = gameObjects[j]->GetTransform();

					// OBBの複雑な回転を考慮した上で、正しい方向へ押し出す！
					tA.position.x -= mtv.x * weightA;
					tA.position.y -= mtv.y * weightA;
					tA.position.z -= mtv.z * weightA;

					tB.position.x += mtv.x * weightB;
					tB.position.y += mtv.y * weightB;
					tB.position.z += mtv.z * weightB;

					// 押し出された方向のスピードを0にする
					if (std::abs(mtv.x) > 0.0001f)
					{
						if (rigidbodyObjectA != nullptr)
						{
							rigidbodyObjectA->velocity.x = 0.0f;
						}

						if (rigidbodyObjectB != nullptr)
						{
							rigidbodyObjectB->velocity.x = 0.0f;
						}
					}

					if (std::abs(mtv.y) > 0.0001f)
					{
						if (rigidbodyObjectA != nullptr)
						{
							rigidbodyObjectA->velocity.y = 0.0f;
						}

						if (rigidbodyObjectB != nullptr)
						{
							rigidbodyObjectB->velocity.y = 0.0f;
						}
					}

					if (std::abs(mtv.z) > 0.0001f)
					{
						if (rigidbodyObjectA != nullptr)
						{
							rigidbodyObjectA->velocity.z = 0.0f;
						}

						if (rigidbodyObjectB != nullptr)
						{
							rigidbodyObjectB->velocity.z = 0.0f;
						}
					}

					// 動いたオブジェクトだけ行列を更新
					if (weightA > 0.0f)
					{
						gameObjects[i]->UpdateTransform();
					}

					if (weightB > 0.0f)
					{
						gameObjects[j]->UpdateTransform();
					}
				}
			}

			// ぶつかっていたらフラグをONにする
			if (hit)
			{
				colA->isColliding = true;
				colB->isColliding = true;
			}
		}
	}

	

}