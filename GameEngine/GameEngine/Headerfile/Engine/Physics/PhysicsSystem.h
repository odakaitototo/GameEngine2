#pragma once
#include <vector>
#include <memory>

// 前方宣言
class GameObject;

class PhysicsSystem
{
public:
	PhysicsSystem() = default;
	~PhysicsSystem() = default;

	// 毎フレーム呼ばれる、物理演算と衝突判定のメインループ
	void Update(const std::vector<std::shared_ptr<GameObject>>& gameObjects);

};
