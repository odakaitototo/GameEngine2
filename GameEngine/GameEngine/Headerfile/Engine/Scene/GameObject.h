#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h> // 数字ライブラリ（座標計算用）
#include <Engine/Graphics/Mesh.h>
#include <Engine/Graphics/Texture.h>
#include <memory>
#include "Engine/Component/ComponentBase.h"
#include <Engine/Component/TransformComponent.h>
#include <Engine/Component/MeshRendererComponent.h>
// 外部ファイル
#include <../SourceFile/Engine/Json/json.hpp> // JSON


using json = nlohmann::json;


class GameObject
{
public:
	GameObject(std::string name);
	~GameObject() {}

	// ゲッター・セッター
	const std::string& GetName() const { return m_name; }
	void SetName(const std::string& name) { m_name = name; }

	//複製機能
	//自分自身のコピーを作成して返す関数宣言
	std::shared_ptr<GameObject> Clone() const;



public: // コンポーネントを追加取得するためのもの (ComponentBase)

	// コンポーネントを追加する機能
	template <class T>
	std::shared_ptr<T> AddComponent()
	{
		auto component = std::make_shared<T>(); // 新しい部品を作る
		component->gameObject = this; // コンポーネントの親がGameObjectだと伝える
		m_component.push_back(component); // リストに追加　Ctrl+Zを使うためにリストの一番後ろに追加

		return component;
	}

	// コンポーネントを取得する機能
	template <class T>
	std::shared_ptr<T> GetComponent() const
	{
		for (auto& c : m_component)
		{
			// dynamic_pointer_castで、探している型(T)と一致するかチェックする
			auto casted = std::dynamic_pointer_cast<T>(c);
			if(casted)
			{
				return casted; // 見つかったら見つかったものを返す
			}
			
		}
		return nullptr; // 見つからなかったら空を返す
	}


public: // コンポーネントの追加

	//////////////////////
	// 
	// TrancformComponent
	// 
	//////////////////////
	TransformComponent& GetTransform()
	{
		return *GetComponent<TransformComponent>();
	}
	const TransformComponent& GetTransform() const
	{
		return *GetComponent<TransformComponent>();
	}


	///////////////////////////
	// 
	// MeshRendererComponent
	// 
	//////////////////////////

	DirectX::XMFLOAT4& GetColor() // 単色
	{
		return GetComponent<MeshRendererComponent>()->color;
	}


	bool& GetUseSolidColor() // 虹色
	{
		return GetComponent<MeshRendererComponent>()->useSolidColor;
	}

	void SetMesh(std::shared_ptr<Mesh>mesh)
	{
		GetComponent<MeshRendererComponent>()->mesh = mesh;
	}

	void SetTexture(std::shared_ptr<Texture>texture)
	{
		GetComponent<MeshRendererComponent>()->texture = texture;
	}

	std::shared_ptr<Texture> GetTexture() const
	{
		return GetComponent<MeshRendererComponent>()->texture;
	}

	void Draw(ID3D11DeviceContext* context);
	



public: // JSON関係

	nlohmann::json ToJson() const;
	void FromJson(const json& JSON, ID3D11Device* device);

private:
	std::string m_name; // オブジェクト名
	
private: // コンポーネント関係
	// このオブジェクトが持っている全てのコンポーネントを管理する箱
	std::vector<std::shared_ptr <ComponentBase>> m_component;

	


};
	




