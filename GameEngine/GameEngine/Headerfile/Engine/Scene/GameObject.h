#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h> // 数字ライブラリ（座標計算用）
#include <Engine/Graphics/Mesh.h>
// 外部ファイル
#include <../SourceFile/Engine/Json/json.hpp> // JSON

using json = nlohmann::json;



// Transform情報をまとめる構造体
struct Transform
{
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f }; // 位置
	DirectX::XMFLOAT3 rotation = { 0.0f,0.0f,0.0f }; // 回転
	DirectX::XMFLOAT3 scale = { 1.0f,1.0f,1.0f }; // 拡大縮小
};

class GameObject
{
public:
	GameObject(std::string name) : m_name(name){}
	~GameObject(){}

	// ゲッター・セッター
	const std::string& GetName() const { return m_name; }
	void SetName(const std::string& name) { m_name = name; }

	Transform& GetTransform() { return m_transform; }

	DirectX::XMFLOAT4& GetColor() { return m_color; } // 色のデータを置き換え参照できるようにする

	bool& GetUseSolidColor() { return m_useSolidColor; } // 単色化虹色かどうかのスイッチを参照する

public: // JSON関係
	// JSONへの書き出し
	json ToJson() const
	{
		// "name"で登録した名前とTransformの情報をペアで管理する
		// j["name"]で呼び出すことで情報を呼び出すことができる
		json j;
		j["name"] = m_name; // 変数 m_name を"name"というキーで登録
		j["transform"] =
		{
			{"position", {m_transform.position.x,m_transform.position.y,m_transform.position.z}},
			{"rotation", {m_transform.rotation.x,m_transform.rotation.y,m_transform.rotation.z}},
			{"scale", {m_transform.scale.x,m_transform.scale.y,m_transform.scale.z}}
		};

		j["color"] = { m_color.x, m_color.y, m_color.z, m_color.w }; // 色のデータ
		j["useSolidColor"] = m_useSolidColor; // 単色モードのON/OFFの情報
		return j;
	}

	// JSONから読み込み
	void FromJson(const json& j)
	{
		m_name = j.at("name").get<std::string>();
		auto& t = j.at("transform");
		m_transform.position = { t["position"][0],t["position"][1], t["position"][2] };
		m_transform.rotation = { t["rotation"][0],t["rotation"][1], t["rotation"][2] };
		m_transform.scale = { t["scale"][0],t["scale"][1] ,t["scale"][2] };

		if (j.contains("color"))
		{
			m_color.x = j["color"][0];
			m_color.y = j["color"][1];
			m_color.z = j["color"][2];
			m_color.w = j["color"][3];

		}

		if (j.contains("useSolidColor"))
		{
			m_useSolidColor = j["useSolidColor"];
		}

	}

public: // メッシュとコンテキスト
	// このオブジェクトが使うメッシュをセットする
	void SetMesh(std::shared_ptr<Mesh> mesh)
	{
		m_mesh = mesh;
	}

	// 描画実行
	void Draw(ID3D11DeviceContext* context)
	{
		if (m_mesh)
		{
			m_mesh->Bind(context); // GPUに指定のメッシュを使うことを伝える
			m_mesh->Draw(context); // 描画開始の命令
		}
	}


private:
	std::string m_name; // オブジェクト名
	Transform m_transform; // 位置・回転・スケール


private: // 色関係
	DirectX::XMFLOAT4 m_color = { 1.0f, 1.0f, 1.0f,1.0f }; // 色のデータ
	bool m_useSolidColor = false;


private: // メッシュとコンテキスト
	std::shared_ptr<Mesh> m_mesh; // メッシュへの共通ポインタ

};