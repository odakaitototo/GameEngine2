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
#include <Engine/Component/ModelRendererComponent.h>
#include <Engine/Component/UIRendererComponent.h>
// 外部ファイル
#include <../SourceFile/Engine/Json/json.hpp> // JSON


using json = nlohmann::json;


class GameObject
{
public:
	GameObject(std::string name);
	~GameObject();

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

	// 複数のコンポーネントを全て取得する機能
	template <class T>
	std::vector<std::shared_ptr<T>> GetComponents() const
	{
		std::vector<std::shared_ptr<T>> result;
		for (auto& c : m_component)
		{
			auto casted = std::dynamic_pointer_cast<T>(c);
			if (casted)
			{
				result.push_back(casted); // 見つかったものを全てリストに詰める
			}
		}
		return result;
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
		auto mesh = GetComponent<MeshRendererComponent>();
		if (mesh)
		{
			return mesh->color;
		}

		auto model = GetComponent<ModelRendererComponent>();
		if (model)
		{
			return model->color;
		}

		static DirectX::XMFLOAT4 dummy = { 1,1,1,1 };
		return dummy;
	}


	bool& GetUseSolidColor() // 虹色
	{
		auto mesh = GetComponent<MeshRendererComponent>();
		if (mesh) return mesh->useSolidColor;
		auto model = GetComponent<ModelRendererComponent>();
		if (model) return model->useSolidColor;

		static bool dummy = false;
		return dummy;
	}

	void SetMesh(std::shared_ptr<Mesh>mesh)
	{
		auto meshRenderer = GetComponent<MeshRendererComponent>();
		if (meshRenderer)
		{
			meshRenderer->mesh = mesh;
		}
	}

	void SetTexture(std::shared_ptr<Texture>texture)
	{
		auto meshRenderer = GetComponent<MeshRendererComponent>();
		if (meshRenderer)
		{
			meshRenderer->texture = texture;
		}

		// Modelの方にもテクスチャをセットできるようにする
		auto modelRenderer = GetComponent<ModelRendererComponent>();
		if (modelRenderer)
		{
			modelRenderer->texture = texture;
		}

		// UI用のテクスチャをセットできるようにする
		auto uiRenderer = GetComponent<UIRendererComponent>();
		if (uiRenderer)
		{
			uiRenderer->texture = texture;
		}
	}

	std::shared_ptr<Texture> GetTexture() const
	{
		auto mesh = GetComponent<MeshRendererComponent>();
		if (mesh) return mesh->texture;
		auto model = GetComponent<ModelRendererComponent>();
		if (model) return model->texture;

		return nullptr;
	}

	void Draw(ID3D11DeviceContext* context);
	



public: // JSON関係

	nlohmann::json ToJson() const;
	void FromJson(const json& JSON, ID3D11Device* device);


	public: // 親子関係
		void SetParent(GameObject* parent); // 親を設定する
		GameObject* GetParent() const
		{
			return m_parent; // 親を取得する
		}
		const std::vector<GameObject*>& GetChildren() const 
		{ 
			return m_children; // 子供たちを取得する
		}

		void AddChild(GameObject* child); // 子供を追加する
		void RemoveChild(GameObject* child); // 子供を外す

		void UpdateTransform(); // 毎フレーム呼んで自分の行列を計算する関数
		void Update(); // コンポーネントのLateUpdateを呼ぶもの
		void LateUpdate(); // コンポーネントのUpdateを呼ぶもの
		void RemoveComponent(std::shared_ptr<ComponentBase> component); // アタッチしたコンポーネントを外すもの

public: // シーン関係
	// シーンの切り替え時に自分を消去させないフラグ
	bool dontDestroyOnLoad = false;




private:
	std::string m_name; // オブジェクト名
	
private: // コンポーネント関係
	// このオブジェクトが持っている全てのコンポーネントを管理する箱
	std::vector<std::shared_ptr <ComponentBase>> m_component;

	
private: // 親子関係

	// 親と子を記憶する変数
	GameObject* m_parent = nullptr; // 自分の親
	std::vector<GameObject*> m_children; // 自分の子供たち


};
	




