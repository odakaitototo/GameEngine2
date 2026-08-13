#include "Engine//System/Physics/RigidbodySystem.h"
#include "Engine//Component/RigidbodyComponent.h"

void RigidbodySystem::Update(std::vector<std::shared_ptr<GameObject>>& gameobjects)
{
	// 全オブジェクトの中から、Rigidbodyを持つものだけ動かす
	for (auto& obj : gameobjects)
	{
		auto rb = obj->GetComponent<RigidbodyComponent>();

		if (rb != nullptr)
		{
			auto& transform = obj->GetTransform();
			float dt = 1.0f / 60.0f; // 1フレームの時間

			// 重力がオンなら落下
			if (rb->useGravity)
			{
				rb->velocity.y -= 9.80f * rb->gravityScale * dt;

				// 重力で動くオブジェクトのスピードの限界値を設定
				if (rb->velocity.y < -1.0f)
				{
					rb->velocity.y = -1.0f;
				}
			}

			// 毎フレーム、抵抗の分だけスピードを削る(1.0 - 抵抗値)
			float damping = 1.0f - (rb->drag * dt);
			if (damping < 0.0f) damping = 0.0f; // マイナスにならないように制限

			rb->velocity.x *= damping;
			rb->velocity.y *= damping;
			rb->velocity.z *= damping;

			// 速度の適用
			if (!rb->freezePosX) // X方向
			{
				transform.position.x += rb->velocity.x;
			}

			if (!rb->freezePosY) // Y方向
			{
				transform.position.y += rb->velocity.y;
			}

			if (!rb->freezePosZ) // Z方向
			{
				transform.position.z += rb->velocity.z;
			}

			// 行列の更新
			obj->UpdateTransform();
		}
	}
}