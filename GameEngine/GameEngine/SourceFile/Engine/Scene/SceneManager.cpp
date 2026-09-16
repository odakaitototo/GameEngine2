#include "Engine/Scene/SceneManager.h"
#include "Engine/Core/Application.h"
#include "../Json/json.hpp"


#include <fstream>
#include <vector>
#include <Engine/Component/ScriptComponent.h>

using json = nlohmann::json;

std::string SceneManager::s_targetScene = "";

void SceneManager::LoadWithLoadingScreen(Application* app, const std::string& targetFilename)
{
	s_targetScene = targetFilename; // 本当に遷移したいシーンを記憶する
	LoadScene(app, "LoadingScene.json"); // ロード画面
}

void SceneManager::SaveScene(Application* app, const std::string& filename)
{
	json root = json::array();

	// DontDestroyOnLoad が true のオブジェクトは、このシーン専用のデータではないので保存しない
	for (int i = 0; i < app->m_gameObjects.size(); i++)
	{
		if (app->m_gameObjects[i]->dontDestroyOnLoad) // dontDestroyOnLoadがtrueのオブジェクトはcontinueで保存しないようにする
		{
			continue;
		}

		json j = app->m_gameObjects[i]->ToJson(); // 次のシーンに継承しなくてよいもの(シーン情報として保存しておくもの)

		int parentIndex = -1;
		GameObject* parent = app->m_gameObjects[i]->GetParent(); // 親子関係の確認　自分の親オブジェクトを探し取得する
		if (parent != nullptr)	// 親がいた場合出席番号を探す
		{
			for (int p = 0; p < app->m_gameObjects.size(); p++) // 全オブジェクトを最初から順番に呼び出し確認する
			{
				if (app->m_gameObjects[p].get() == parent) // 見ているオブジェクトのポインタが探しているオブジェクトのポインタと同じかどうかを確認
				{
					parentIndex = p; // 親の出席番号を記憶する
					break;
				}
			}
		}

		j["parentIndex"] = parentIndex; // parentIndexとして親の出席番号を記憶させる
		root.push_back(j); // 完成したオブジェクトのデータをセーブデータ全体の最後尾に追加し保存する
	}

	// 指定された名前でファイルを作成
	std::ofstream ofs(filename);

	if (ofs) // ファイルが無事に開けたかどうか確認
	{
		ofs << root.dump(4); // Jsonデータにして書き込み　dump(4)4文字分の空白でインデントや字下げがされる
	}
}

static std::string s_nextScene = "";
static bool s_shouldLoadScene = false;

// シーン遷移が一瞬過ぎてPlayerの入力が次のシーンに影響してしまうのでシーンの読み込み予約
void SceneManager::LoadScene(Application* app, const std::string& filename)
{
	s_nextScene = filename;
	s_shouldLoadScene = true;
}

void SceneManager::ExecuteLoadScene(Application* app)
{
	// 読み込み予約が無ければ何もしない
	if (!s_shouldLoadScene)
	{
		return;
	}


	s_shouldLoadScene = false;

	std::string filename = s_nextScene;


	std::ifstream ifs(filename);
	if (!ifs)
	{
		return;
	}

	json root;

	try
	{
		ifs >> root;
	}
	catch (...)
	{
		return;
	}

	if (!root.is_array())
	{
		return;
	}

	/////////////////////////////////////
	//
	// シーン消去DontDestroyOnLoadの処理
	//
	//////////////////////////////////////

	std::vector<std::shared_ptr<GameObject>> keepObjects;

	for (auto& obj : app->m_gameObjects)
	{
		if (obj->dontDestroyOnLoad) // dontDestroyOnLoadがtrueなら避難用リストに入れておく
		{
			keepObjects.push_back(obj);
		}
	}

	// 今のシーンを消去
	app->m_gameObjects.clear();

	// 避難させておいたオブジェクトを新しいシーンに出す
	app->m_gameObjects = keepObjects;

	// JSONから新しいシーンのオブジェクトを生成する
	for (const auto& j : root)
	{
		if (!j.is_object()) continue;

		auto obj = std::make_shared<GameObject>("");
		obj->FromJson(j, app->m_dx.GetDevice());
		// スカイドームの時は球体のメッシュで復元する
		if (obj->GetName() == "SkyDome")
		{
			obj->SetMesh(app->m_skyMesh);
		}
		else
		{
			obj->SetMesh(app->m_commonMesh);
		}
		app->m_gameObjects.push_back(obj);
	}

	// 親子関係の結び直し
	for (int i = 0; i < root.size(); i++)
	{
		if (root[i].is_object() && root[i].contains("parentIndex"))
		{
			if (root[i]["parentIndex"].is_number())
			{
				int parentIndex = root[i]["parentIndex"];
				if (parentIndex >= 0)
				{
					// 避難したオブジェクトの数だけ、出席番号が後ろにズレているので補正する
					int offset = (int)keepObjects.size();
					int actualParent = parentIndex + offset;
					int actualChild = i + offset;

					if (actualParent < app->m_gameObjects.size() && actualChild < app->m_gameObjects.size())
					{

						auto savedPos = app->m_gameObjects[actualChild]->GetTransform().position;
						auto savedRot = app->m_gameObjects[actualChild]->GetTransform().rotation;
						auto savedScl = app->m_gameObjects[actualChild]->GetTransform().scale;

						app->m_gameObjects[actualChild]->SetParent(app->m_gameObjects[actualParent].get());

						app->m_gameObjects[actualChild]->GetTransform().position = savedPos;
						app->m_gameObjects[actualChild]->GetTransform().rotation = savedRot;
						app->m_gameObjects[actualChild]->GetTransform().scale = savedScl;

						app->m_gameObjects[actualChild]->UpdateTransform();
					}
				}
			}
		}
	}
	// 新しくロードされたオブジェクトの Start() を呼び出す処理
	int offset = (int)keepObjects.size();
	for (int i = offset; i < app->m_gameObjects.size(); i++)
	{
		// オブジェクトにアタッチされている全てのスクリプトを取得
		auto scripts = app->m_gameObjects[i]->GetComponents<ScriptComponent>();
		for (auto& script : scripts)
		{
			script->Start(); // 新しいシーンが始まったのでStartを呼ぶ
		}
	}
}