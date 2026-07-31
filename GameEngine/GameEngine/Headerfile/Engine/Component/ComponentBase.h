#pragma once

// じぶんがどのオブジェクトについているかを知るための前方宣言
class GameObject; // GameObjectのポインターを使えるようにするため

class ComponentBase // これから作るコンポーネントの親になるもの。今後作られるコンポーネントには{}内の内容が引き継がれる
{
public:
	ComponentBase() = default;
	virtual ~ComponentBase() = default; // ポリモーフィズムのための仮想。virtualを書くことで子クラスそれぞれの処理を実行するようになる

	// この部品がくっついている親(GameObject)へのポインタ
	GameObject* gameObject = nullptr;

	// 毎フレーム呼ばれる更新処理
	virtual void Update(){}
	virtual void LateUpdate(){} // 行列計算後に呼ばれる後処理
};
