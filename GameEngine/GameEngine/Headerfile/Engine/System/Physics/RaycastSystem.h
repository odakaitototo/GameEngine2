#pragma once
#include <vector>
#include <memory>
#include <DirectXMath.h>

class GameObject;

// レイ本体
struct Ray
{
	DirectX::XMFLOAT3 origin; // レイの発射地点
	DirectX::XMFLOAT3 direction; // レイが飛ぶ方向
};

// レイが当たった結果
struct RaycastHit
{
	bool isHit = false; // 何かに当たったかどうか
	float distance = 0.0f; // 発射地点からどれだけ先で当たったか
	std::shared_ptr<GameObject> hitObject = nullptr; // 当たったオブ軸との判別
};

// レイキャストの管理クラス
class RaycastSystem
{
public:

	// レーザー発射関数
	static RaycastHit Raycast(const Ray& ray, const std::vector<std::shared_ptr<GameObject>>& gameObjects);
};