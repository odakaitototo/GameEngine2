#include "Engine/Scene/SceneManager.h"
#include "Engine/Core/Application.h"
#include "../Json/json.hpp"


#include <fstream>
#include <vector>

using json = nlohmann::json;

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


void SceneManager::LoadScene(Application* app, const std::string& filename)
{
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
		obj->SetMesh(app->m_commonMesh);
		app->m_gameObjects.push_back(obj);
	}

	// 親子関係の結び直し
	for (int i = 0; i < root.size(); i++)
	{
		if (root[i].is_object() && root[i].contains("ParentIndex"))
		{
			if (root[i]["ParentIndex"].is_number())
			{
				int parentIndex = root[i]["ParentIndex"];
				if (parentIndex >= 0)
				{
					// 避難したオブジェクトの数だけ、出席番号が後ろにズレているので補正する
					int offset = (int)keepObjects.size();
					int actualParent = parentIndex + offset;
					int actualChild = i + offset;

					if (actualParent < app->m_gameObjects.size() && actualChild < app->m_gameObjects.size())
					{
						app->m_gameObjects[actualChild]->SetParent(app->m_gameObjects[actualParent].get());
					}
				}
			}
		}
	}
}