#include "Engine/Scene/GameObject.h"

// 自分自身のコピー（分身）を作成して返す関数
std::shared_ptr<GameObject> GameObject::Clone()const
{
	// 新しいオブジェクトの生成
	auto clone = std::make_shared<GameObject>(m_name + "_Copy");

	// 独立した値のコピー
	clone->m_transform = this->m_transform;
	clone->m_color = this->m_color;
	clone->m_useSolidColor = this->m_useSolidColor;

	// リソースの共有
	clone->m_mesh = this->m_mesh;
	clone->m_texture = this->m_texture;

	return clone;
} 
// JSONへの書き出し
nlohmann::json GameObject::ToJson() const
{
	json JSON;
	JSON["name"] = m_name;
	JSON["transform"] =
	{
		{"position", {m_transform.position.x,m_transform.position.y,m_transform.position.z}},
		{"rotation", {m_transform.rotation.x,m_transform.rotation.y,m_transform.rotation.z}},
		{"scale", {m_transform.scale.x, m_transform.scale.y, m_transform.scale.z}},
	};

	JSON["color"] = { m_color.x, m_color.y, m_color.z, m_color.w };
	JSON["useSolidcolor"] = m_useSolidColor;

	if (m_texture != nullptr)
	{
		JSON["texturePath"] = m_texture->GetFilePath();
	}
	else
	{
		JSON["TexturePath"] = "";
	}
	return JSON;
}
	
void GameObject::FromJson(const json& JSON, ID3D11Device* device)
{
	m_name = JSON.at("name").get<std::string>();

	auto& transform = JSON.at("transform");
	m_transform.position = { transform["position"][0], transform["position"][1],transform["position"][2] };
	m_transform.rotation = { transform["rotation"][0],transform["rotation"][1],transform["rotation"][2] };
	m_transform.scale    = { transform["scale"][0],transform["scale"][1],transform["scale"][2] };

	if (JSON.contains("color"))
	{
		m_color.x = JSON["color"][0];
		m_color.y = JSON["color"][1];
		m_color.z = JSON["color"][2];
		m_color.w = JSON["color"][3];
	}

	if (JSON.contains("useSolidColor"))
	{
		m_useSolidColor = JSON["useSolidColor"];
	}

	if (JSON.contains("texturePath") && JSON["texturePath"] != "")
	{
		std::string path = JSON["texturePath"];

		auto loadedTex = std::make_shared<Texture>();
		if(loadedTex->Load(device,path))
		{
			m_texture = loadedTex;
		}
	}
	else
	{
		m_texture = nullptr;
	}

	

}

void GameObject::Draw(ID3D11DeviceContext* context)
{
	if (m_mesh)
	{
		m_mesh->Bind(context); // GPUに指定のメッシュを使うことを伝える
		m_mesh->Draw(context); // 描画開始の命令
	}
}

	
