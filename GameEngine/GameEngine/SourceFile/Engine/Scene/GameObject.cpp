#include "Engine/Scene/GameObject.h"
#include "Engine/Component/TransformComponent.h"
#include "Engine/Component/MeshRendererComponent.h"
#include "Engine/Component/AABBColliderComponent.h"
#include "Engine/Component/OBBColliderComponent.h"
#include "ImGuizmo.h"
#include "Engine/Component/ColliderBase.h"

#include <algorithm>



// GameObjectが生まれた瞬間の処理
GameObject::GameObject(std::string name) : m_name(name)
{
	// 生まれた瞬間に、自分自身に必要なコンポーネントをセットする
	AddComponent<TransformComponent>();
	AddComponent<MeshRendererComponent>();
	
}

GameObject::~GameObject()
{
	// もし自分に親がいたら、親の子供リストから自分を消去してもらう
	if (m_parent != nullptr)
	{
		m_parent->RemoveChild(this);
	}

	// もし自分に子供がいたら子供たちを独立させる
	auto childrenCopy = m_children;
	for (auto* child : childrenCopy)
	{
		child->SetParent(nullptr);
	}

}

void GameObject::Update()
{
	 // コンポーネントのUpdateだけ呼ぶ
	for (auto& comp : m_component)
	{
		if (comp != nullptr)
		{
			comp->Update();
		}
	}
}


void GameObject::LateUpdate()
{
	// コンポーネントのLateUpdateだけを呼ぶ
	for (auto& comp : m_component)
	{
		if (comp != nullptr)
		{
			comp->LateUpdate();
		}
	}
}

// 自分自身のコピー（分身）を作成して返す関数
std::shared_ptr<GameObject> GameObject::Clone()const
{
	// 新しいオブジェクトの生成
	auto clone = std::make_shared<GameObject>(m_name + "_Copy");

	// 独立した値のコピー
	auto& myTransform = this->GetTransform();
	auto& cloneTransform = clone->GetTransform();
	cloneTransform.position = myTransform.position;
	cloneTransform.rotation = myTransform.rotation;
	cloneTransform.scale    = myTransform.scale;

	auto myRender = this->GetComponent<MeshRendererComponent>();
	auto cloneRender = clone->GetComponent<MeshRendererComponent>();
	cloneRender->color = myRender->color;
	cloneRender->useSolidColor = myRender->useSolidColor;
	cloneRender->mesh = myRender->mesh;
	cloneRender->texture = myRender->texture;
	

	return clone;
} 
// JSONへの書き出し
nlohmann::json GameObject::ToJson() const
{
	json JSON;
	JSON["name"] = m_name;
	auto& transform = GetTransform();
	JSON["transform"] =
	{
		{"position", {transform.position.x,transform.position.y,transform.position.z}},
		{"rotation", {transform.rotation.x,transform.rotation.y,transform.rotation.z}},
		{"scale", {transform.scale.x, transform.scale.y, transform.scale.z}},
	};

	auto render = GetComponent<MeshRendererComponent>();
	JSON["color"] = { render->color.x,render->color.y, render->color.z, render->color.w };
	JSON["useSolidColor"] = render->useSolidColor;

	if (render->texture != nullptr)
	{
		JSON["texturePath"] = render->texture->GetFilePath();
	}
	else
	{
		JSON["texturePath"] = "";
	}

	// 当たり判定の情報
	auto collider = GetComponent<ColliderBase>();
	if (collider != nullptr)
	{
		nlohmann::json colliderJson;

		// AABBの保存
		if (collider->GetColliderType() == ColliderType::AABB)
		{
			auto aabb = std::static_pointer_cast<AABBColliderComponent>(collider);
			auto& box = aabb->localBoundingBox;

			colliderJson["Type"] = "AABB";
			colliderJson["Center"] = { box.Center.x, box.Center.y , box.Center.z };
			colliderJson["Extents"] = { box.Extents.x, box.Extents.y, box.Extents.z };
		}
		// OBBの保存
		else if (collider->GetColliderType() == ColliderType::OBB)
		{
			auto obb = std::static_pointer_cast<OBBColliderComponent>(collider);
			auto& box = obb->localBoundingBox;

			colliderJson["Type"] = "OBB";
			colliderJson["Center"] = { box.Center.x, box.Center.y, box.Center.z };
			colliderJson["Extents"] = { box.Extents.x, box.Extents.y, box.Extents.z };
			colliderJson["Orientation"] = { box.Orientation.x, box.Orientation.y, box.Orientation.z, box.Orientation.w };
		}

		JSON["Collider"] = colliderJson; // JSONデータに追加
	}



	return JSON;
}
	
void GameObject::FromJson(const json& JSON, ID3D11Device* device)
{
	m_name = JSON.at("name").get<std::string>();
	auto& transform = GetTransform();
	auto& transformData = JSON.at("transform");
	transform.position = { transformData["position"][0], transformData["position"][1],transformData["position"][2] };
	transform.rotation = { transformData["rotation"][0],transformData["rotation"][1],transformData["rotation"][2] };
	transform.scale    = { transformData["scale"][0],transformData["scale"][1],transformData["scale"][2] };

	auto render = GetComponent<MeshRendererComponent>();
	if (JSON.contains("color"))
	{
		render->color.x = JSON["color"][0];
		render->color.y = JSON["color"][1];
		render->color.z = JSON["color"][2];
		render->color.w = JSON["color"][3];
	}

	if (JSON.contains("useSolidColor"))
	{
		render->useSolidColor = JSON["useSolidColor"];
	}

	if (JSON.contains("texturePath") && JSON["texturePath"] != "")
	{
		std::string path = JSON["texturePath"];
		auto loadedTex = std::make_shared<Texture>();
		if (loadedTex->Load(device, path))
		{
			render->texture = loadedTex;
		}
	}
	else
	{
		render->texture = nullptr;
	}

	// Colliderの読み込み
	if (JSON.contains("Collider"))
	{
		auto colliderJson = JSON["Collider"];
		std::string type = colliderJson["Type"];

		// AABBの復元
		if (type == "AABB")
		{
			auto aabb = AddComponent<AABBColliderComponent>();

			aabb->localBoundingBox.Center = DirectX::XMFLOAT3(colliderJson["Center"][0], colliderJson["Center"][1], colliderJson["Center"][2]);

			aabb->localBoundingBox.Extents = DirectX::XMFLOAT3(colliderJson["Extents"][0], colliderJson["Extents"][1], colliderJson["Extents"][2]);
		}
		// OBBの復元
		else if (type == "OBB")
		{
			auto obb = AddComponent<OBBColliderComponent>();

			obb->localBoundingBox.Center = DirectX::XMFLOAT3(colliderJson["Center"][0], colliderJson["Center"][1], colliderJson["Center"][2]);

			obb->localBoundingBox.Extents = DirectX::XMFLOAT3(colliderJson["Extents"][0], colliderJson["Extents"][1], colliderJson["Extents"][2]);

			obb->localBoundingBox.Orientation = DirectX::XMFLOAT4(colliderJson["Oriention"][0], colliderJson["Oriention"][1], colliderJson["Oriention"][2],colliderJson["Oriention"][3]);
		}
	}

}

void GameObject::Draw(ID3D11DeviceContext* context)
{
	GetComponent<MeshRendererComponent>()->Draw(context);
}

void GameObject::UpdateTransform()
{
	auto& trans = GetTransform();

	// 自分自身の行列を作る
	DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(trans.scale.x, trans.scale.y, trans.scale.z);
	
	float radX = DirectX::XMConvertToRadians(trans.rotation.x);
	float radY = DirectX::XMConvertToRadians(trans.rotation.y);
	float radZ = DirectX::XMConvertToRadians(trans.rotation.z);
	
	// 各軸の回転行列をバラバラに作成する
	DirectX::XMMATRIX rotX = DirectX::XMMatrixRotationX(radX);
	DirectX::XMMATRIX rotY = DirectX::XMMatrixRotationY(radY);
	DirectX::XMMATRIX rotZ = DirectX::XMMatrixRotationZ(radZ);

	DirectX::XMMATRIX rotMat = rotX * rotY * rotZ;



	DirectX::XMMATRIX transMat = DirectX::XMMatrixTranslation(trans.position.x, trans.position.y, trans.position.z);

	DirectX::XMMATRIX localMatrix = scaleMat * rotMat * transMat;

	// 親がいる場合は自分の行列 * 親のワールド行列を計算する
	DirectX::XMMATRIX worldMat;
	if (m_parent)
	{
		// 親の最新のワールド行列を取得
		DirectX::XMMATRIX parentWorld = DirectX::XMLoadFloat4x4(&m_parent->GetTransform().worldMatrix);
		worldMat = localMatrix * parentWorld; // ここで行列の掛け算
	}
	else
	{
		worldMat = localMatrix; // 親がいない場合はそのまま
	}



	// 計算結果を保存する
	DirectX::XMStoreFloat4x4(&trans.worldMatrix, worldMat);

	// 親が動いたら子供も動くようにするために子供も再帰的に更新する
	for (auto* child : m_children)
	{
		child->UpdateTransform();
	}

}

// 親子関係の付けはずし
void GameObject::SetParent(GameObject* parent)
{
	// 結合する前に自分の座標を記憶しておく
	UpdateTransform();
	DirectX::XMMATRIX oldWorldMat = DirectX::XMLoadFloat4x4(&GetTransform().worldMatrix);

	// 結合解除
	if (m_parent)
	{

		m_parent->RemoveChild(this);

	}



	m_parent = parent;

	// 新しいオブジェクトと結合する
	if (m_parent)
	{
		m_parent->AddChild(this);
		m_parent->UpdateTransform(); // 親オブジェクトの座標を更新する
	}

	// 子オブジェクトの座標がずれないように親オブジェクトから見たローカル座標を逆算する
	DirectX::XMMATRIX newLocalMat;

	if (m_parent)
	{
		// 新しい親オブジェクトのワールド行列の逆行列を作り自分の絶対座標に掛け算する
		DirectX::XMVECTOR det;
		DirectX::XMMATRIX parentWorldMat = DirectX::XMLoadFloat4x4(&m_parent->GetTransform().worldMatrix);
		DirectX::XMMATRIX invParentWorldMat = DirectX::XMMatrixInverse(&det, parentWorldMat);

		newLocalMat = oldWorldMat * invParentWorldMat;
	}
	else
	{
		// 親がいなくなった場合絶対座標がそのままローカル座標に残る
		newLocalMat = oldWorldMat;
	}

	// 計算した新しい行列から、位置・回転・スケールの数値を抜き出してTransformに上書きする
	DirectX::XMFLOAT4X4 localFloat;
	DirectX::XMStoreFloat4x4(&localFloat, newLocalMat);

	float t[3], r[3], s[3];
	ImGuizmo::DecomposeMatrixToComponents(&localFloat.m[0][0], t, r, s);

	auto& trans = GetTransform();
	trans.position = { t[0], t[1], t[2] };
	trans.rotation = { r[0], r[1], r[2] };
	trans.scale    = { s[0], s[1], s[2] };

	// 最後に新しいTransformでもう一度行列を更新
	UpdateTransform();
}

void GameObject::AddChild(GameObject* child)
{
	m_children.push_back(child);
}

void GameObject::RemoveChild(GameObject* child)
{
	// リストの中から自分を探して消す
	auto it = std::find(m_children.begin(), m_children.end(), child);
	if (it != m_children.end())
	{
		m_children.erase(it);
	}
}

	
