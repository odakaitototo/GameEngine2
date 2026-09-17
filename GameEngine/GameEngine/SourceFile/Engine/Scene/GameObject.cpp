#include "Engine/Scene/GameObject.h"
#include "Engine/Component/TransformComponent.h"
#include "Engine/Component/MeshRendererComponent.h"
#include "Engine/Component/AABBColliderComponent.h"
#include "Engine/Component/OBBColliderComponent.h"
#include "ImGuizmo.h"
#include "Engine/Component/ColliderBase.h"
#include "Engine/Component/RigidbodyComponent.h"
#include "Engine/Component/ModelRendererComponent.h"
#include "Engine/Component/SkyDomeController.h"

// 自作スクリプトを呼び出す場所
#include "Game/Script/EngineTestScriipt/TestController.h"

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
	cloneTransform.scale = myTransform.scale;

	// 元のオブジェクトが持っているコンポーネントに合わせてコピーする
	// MeshRendererのコピー
	auto myMeshRender = this->GetComponent<MeshRendererComponent>();
	if (myMeshRender)
	{
		auto cloneMeshRender = clone->GetComponent<MeshRendererComponent>();
		if (!cloneMeshRender) cloneMeshRender = clone->AddComponent<MeshRendererComponent>(); // 無ければ付ける

		cloneMeshRender->color = myMeshRender->color;
		cloneMeshRender->useSolidColor = myMeshRender->useSolidColor;
		cloneMeshRender->mesh = myMeshRender->mesh;
		cloneMeshRender->texture = myMeshRender->texture;
	}
	else
	{
		// Meshが無い場合は、初期状態で付いているMeshRendererを外しておく
		auto cloneMeshRender = clone->GetComponent<MeshRendererComponent>();
		if (cloneMeshRender) clone->RemoveComponent(cloneMeshRender);
	}

	// ModelRendererのコピー
	auto myModelRender = this->GetComponent<ModelRendererComponent>();
	if (myModelRender)
	{
		auto cloneModelRender = clone->AddComponent<ModelRendererComponent>();
		cloneModelRender->color = myModelRender->color;
		cloneModelRender->useSolidColor = myModelRender->useSolidColor;
		cloneModelRender->model = myModelRender->model; // 同じModelを使い回す（メモリ節約）
		cloneModelRender->texture = myModelRender->texture;
	}

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

	// どちらのRendererがついているか確認して色・テクスチャを保存
	DirectX::XMFLOAT4 saveColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	bool saveSolid = false;
	std::string texPath = "";

	// モデルかメッシュのどちらかを持っているかを判定するためのフラグ
	bool hasRenderer = false;
	std::string rendererType = "None"; // どのコンポーネントを付けるべきか記憶

	auto meshR = GetComponent<MeshRendererComponent>();
	if (meshR)
	{
		saveColor = meshR->color;
		saveSolid = meshR->useSolidColor;
		if (meshR->texture) texPath = meshR->texture->GetFilePath();
		hasRenderer = true;
		rendererType = "Mesh";
	}
	else
	{
		auto modelRenderer = GetComponent<ModelRendererComponent>();
		if (modelRenderer)
		{
			saveColor = modelRenderer->color;
			saveSolid = modelRenderer->useSolidColor;
			if (modelRenderer->texture) texPath = modelRenderer->texture->GetFilePath();
			hasRenderer = true;
			rendererType = "Model";

			 // FBXのファイルパスの保存
			if (modelRenderer->model != nullptr)
			{
				JSON["ModelPath"] = modelRenderer->model->GetFilePath();
			}
		}
		else // UIコンポーネント用のセーブ処理
		{
			auto uiRenderer = GetComponent<UIRendererComponent>();
			if (uiRenderer)
			{
				saveColor = uiRenderer->color;
				if (uiRenderer->texture) texPath = uiRenderer->texture->GetFilePath();
				hasRenderer = true;
				rendererType = "UI"; // 種類をUIとして記録
				// UI固有のパラメータ（位置とサイズ）を保存
				JSON["ui_position"] = { uiRenderer->position.x, uiRenderer->position.y };
				JSON["ui_size"] = { uiRenderer->size.x, uiRenderer->size.y };
			}
		}
	}

	// レンダラー情報がある場合のみJSONに書き込む
	if (hasRenderer)
	{
		JSON["RendererType"] = rendererType; // 再読み込み時にどちらのコンポーネントか判別するため
		JSON["color"] = { saveColor.x, saveColor.y, saveColor.z, saveColor.w };
		JSON["useSolidColor"] = saveSolid;
		JSON["texturePath"] = texPath;
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

	// Rigidbodyの保存
	auto rb = GetComponent<RigidbodyComponent>();
	if (rb != nullptr)
	{
		nlohmann::json rbJson;
		rbJson["useGravity"] = rb->useGravity;
		rbJson["gravityScale"] = rb->gravityScale;
		rbJson["drag"] = rb->drag;
		rbJson["freezePosX"] = rb->freezePosX;
		rbJson["freezePosY"] = rb->freezePosY;
		rbJson["freezePosZ"] = rb->freezePosZ;
		JSON["Rigidbody"] = rbJson;
	}

	nlohmann::json scriptsArray = nlohmann::json::array();
	for (auto& comp : m_component)
	{
		auto script = std::dynamic_pointer_cast<ScriptComponent>(comp);
		if (script != nullptr)
		{
			nlohmann::json scriptJson;
			scriptJson["ScriptName"] = script->GetScriptName();

			script->SaveToJson(scriptJson);

			scriptsArray.push_back(scriptJson);
		}
	}
	if (!scriptsArray.empty())
	{
		JSON["Scripts"] = scriptsArray;
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
	transform.scale = { transformData["scale"][0],transformData["scale"][1],transformData["scale"][2] };

	// 保存されたレンダラーの種類に応じてコンポーネントを設定
	if (JSON.contains("RendererType"))
	{
		std::string type = JSON["RendererType"];

		// 色とテクスチャの読み込み用の一時変数
		DirectX::XMFLOAT4 loadColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		bool loadSolid = false;
		std::shared_ptr<Texture> loadTex = nullptr;

		if (JSON.contains("color"))
		{
			loadColor = { JSON["color"][0], JSON["color"][1], JSON["color"][2], JSON["color"][3] };
		}
		if (JSON.contains("useSolidColor"))
		{
			loadSolid = JSON["useSolidColor"];
		}
		if (JSON.contains("texturePath") && JSON["texturePath"] != "")
		{
			std::string path = JSON["texturePath"];
			loadTex = std::make_shared<Texture>();
			if (!loadTex->Load(device, path))
			{
				loadTex = nullptr; // 読み込み失敗時はnullにする
			}
		}

		// コンポーネントへの適用
		if (type == "Mesh")
		{
			// MeshRendererは初期化時に付いているのでそのまま使う
			auto meshR = GetComponent<MeshRendererComponent>();
			if (meshR)
			{
				meshR->color = loadColor;
				meshR->useSolidColor = loadSolid;
				meshR->texture = loadTex;
			}
		}
		else if (type == "Model")
		{
			// Modelの場合は初期搭載のMeshを消してModelを付ける
			auto meshRenderer = GetComponent<MeshRendererComponent>();
			if (meshRenderer) RemoveComponent(meshRenderer);

			auto modelRenderer = AddComponent<ModelRendererComponent>();
			modelRenderer->color = loadColor;
			modelRenderer->useSolidColor = loadSolid;
			modelRenderer->texture = loadTex;

			// 保存されたパスからFBXを読み込み直す
			if (JSON.contains("ModelPath"))
			{
				std::string modelPath = JSON["ModelPath"];
				auto newModel = std::make_shared<Model>();
				if (newModel->Load(device, modelPath))
				{
					modelRenderer->model = newModel;
				}
			}
		}
		else if (type == "UI") // UIコンポーネント用のロード処理
		{
			// 最初から付いているMeshを外してからUIを付ける
			auto meshRenderer = GetComponent<MeshRendererComponent>();
			if (meshRenderer)
			{
				RemoveComponent(meshRenderer);
			}

			auto uiRenderer = AddComponent<UIRendererComponent>();
			uiRenderer->color = loadColor;
			uiRenderer->texture = loadTex;


			// JSONに保存された位置とサイズを復元
			if (JSON.contains("ui_position"))
			{
				uiRenderer->position = { JSON["ui_position"][0], JSON["ui_position"][1] };
			}
			if (JSON.contains("ui_size"))
			{
				uiRenderer->size = { JSON["ui_size"][0], JSON["ui_size"][1] };
			}
		}
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

			obb->localBoundingBox.Orientation = DirectX::XMFLOAT4(colliderJson["Orientation"][0], colliderJson["Orientation"][1], colliderJson["Orientation"][2], colliderJson["Orientation"][3]);
		}
	}

	// Rigidbodyの復元
	if (JSON.contains("Rigidbody"))
	{
		auto rb = AddComponent<RigidbodyComponent>();
		auto rbJson = JSON["Rigidbody"];

		if (rbJson.contains("useGravity")) rb->useGravity = rbJson["useGravity"];
		if (rbJson.contains("gravityScale")) rb->gravityScale = rbJson["gravityScale"];
		if (rbJson.contains("drag")) rb->drag = rbJson["drag"];
		if (rbJson.contains("freezePosX")) rb->freezePosX = rbJson["freezePosX"];
		if (rbJson.contains("freezePosY")) rb->freezePosY = rbJson["freezePosY"];
		if (rbJson.contains("freezePosZ")) rb->freezePosZ = rbJson["freezePosZ"];
	}

	if (JSON.contains("ScriptComponent"))
	{
		auto scriptJson = JSON["ScriptComponent"];
		std::string scriptName = scriptJson["ScriptName"];

		// 保存された名前を見て、該当する自作スクリプトをアタッチする！
		if (scriptName == "TestController") AddComponent<TestController>();

		// 今後自作のスクリプトを作ったらここに追加していくelse ifで
		else if (scriptName == "SKyDomeController") AddComponent<SkyDomeController>();
		
		
		
	}


	if (JSON.contains("Scripts"))
	{
		for (const auto& scriptJson : JSON["Scripts"])
		{
			std::string scriptName = scriptJson["ScriptName"];
			if (scriptName == "TestController") AddComponent<TestController>();
			// 今後新しいスクリプトを追加していく
			
			else if (scriptName == "SkyDomeController") AddComponent<SkyDomeController>();
			
		}
	}

}

void GameObject::Draw(ID3D11DeviceContext* context)
{
	// メッシュを持っていればメッシュ描画
	auto meshRenderer = GetComponent<MeshRendererComponent>();
	if (meshRenderer)
	{
		meshRenderer->Draw(context);
		return;
	}

	// モデルを持っている場合はモデルを描画
	auto modelRenderer = GetComponent<ModelRendererComponent>();
	if (modelRenderer && modelRenderer->model != nullptr)
	{
		modelRenderer->model->Draw(context);
		return;
	}
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
void GameObject::SetParent(GameObject* parent, bool keepWorldPosition)
{
	// 循環参照(無限ループ)を防止するもの
	GameObject* checkNode = parent;
	while (checkNode != nullptr)
	{
		if (checkNode == this)
		{

			return;
		}

		checkNode = checkNode->GetParent();
	}




	DirectX::XMMATRIX oldWorldMat;


	if (keepWorldPosition) // ワールド座標を維持する場合のみ元の行列を記憶
	{
		// 結合する前に自分の座標を記憶しておく
		UpdateTransform();
		oldWorldMat = DirectX::XMLoadFloat4x4(&GetTransform().worldMatrix);

	}

	

	// 結合解除
	if (m_parent) // 新しい親に結合した際に前の親と座標の取り合いになるから
	{

		m_parent->RemoveChild(this);

	}



	m_parent = parent;

	// 新しいオブジェクトと結合する
	if (m_parent)
	{
		m_parent->AddChild(this);
		if (keepWorldPosition)
		{
			m_parent->UpdateTransform(); // 親オブジェクトの座標を更新する
		}
	}

	if (keepWorldPosition) // trueの時だけ実行
	{
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
		trans.scale = { s[0], s[1], s[2] };

	}
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

////////////////////////
//
// コンポーネントの取り外し
// 
/////////////////////////	

void GameObject::RemoveComponent(std::shared_ptr<ComponentBase> component)
{
	if (component == nullptr) // コンポーネントがついていなかったら何もしない
	{
		return;
	}

	// m_componentから一致するものを探して消す
	m_component.erase(
		std::remove(m_component.begin(), m_component.end(), component),
		m_component.end()
	);
}
