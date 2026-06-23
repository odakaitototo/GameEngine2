#include "Engine/Scene/GameObject.h"
#include "Engine/Component/TransformComponent.h"
#include "Engine/Component/MeshRendererComponent.h"



// GameObjectが生まれた瞬間の処理
GameObject::GameObject(std::string name) : m_name(name)
{
	// 生まれた瞬間に、自分自身に必要なコンポーネントをセットする
	AddComponent<TransformComponent>();
	AddComponent<MeshRendererComponent>();
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
	JSON["UseSolidColor"] = render->useSolidColor;

	if (render->texture != nullptr)
	{
		JSON["texturePath"] = render->texture->GetFilePath();
	}
	else
	{
		JSON["texturePath"] = "";
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

}

void GameObject::Draw(ID3D11DeviceContext* context)
{
	GetComponent<MeshRendererComponent>()->Draw(context);
}

	
