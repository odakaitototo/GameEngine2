#include "Engine/Core/EditorUI.h"
#include "Engine/Core/Application.h"
#include <imgui.h>
#include <string>

void EditorUI::Draw(Application* app)
{
    // Hierarchyウィンドウ（左側に配置）
    ImGui::SetNextWindowPos(ImVec2(400, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Hierarchy");

    if (ImGui::Button("Save Scene"))
    {
        app->SaveScene("scene.txt"); // シーンを保存
    }
    if (ImGui::Button("Load Scene"))
    {
        app->LoadScene("scene.txt"); // シーンを読み込み
    }
    ImGui::Separator();
    // オブジェクト作成ボタン
    if (ImGui::Button("Create Empty"))
    {
        // 新しいオブジェクトを作成してリストに追加
        std::string name = "GameObject" + std::to_string(app->m_gameObjects.size());
        auto newObj = std::make_shared<GameObject>(name);

        newObj->SetMesh(app->m_commonMesh); // 新しく作ったオブジェクトに、共通メッシュの住所を教える

        app->m_gameObjects.push_back(newObj);
    }
    ImGui::Separator();
    if (ImGui::Button("Instantiate Prefab"))
    {
        app->InstantiatePrefab("GameObject0.pfb"); // プレハブを読み込んでシーンに追加
    }
    ImGui::Separator();
    ImGui::Text("Scene Objects");
    ImGui::Separator(); // 区切るための線
   
    for (int i = 0; i < app->m_gameObjects.size(); i++)
    {
        bool isSelected = (app->m_selectedObjectIndex == i);

        std::string displayLadel = app->m_gameObjects[i]->GetName() + "##" + std::to_string(i); // クローンからクローンを作る際にIDが変わるようにした。

        if (ImGui::Selectable(displayLadel.c_str(), isSelected))
        {
            app->m_selectedObjectIndex = i; // 選択したオブジェクトの番号を保存
        }
    }

    ImGui::End();

    // Inspectorウィンドウ（右側に配置）
    ImGui::SetNextWindowPos(ImVec2(600, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Inspector"); // ウィンドウ名
    if (app->m_selectedObjectIndex != -1 && app->m_selectedObjectIndex < app->m_gameObjects.size())
    {
        auto& obj = app->m_gameObjects[app->m_selectedObjectIndex];

        // 名前編集
        char buf[128];
        strcpy_s(buf, obj->GetName().c_str());
        if (ImGui::InputText("Name", buf, sizeof(buf)))
        {
            obj->SetName(buf);
        }
        ImGui::Separator();

        // Transform編集（ここが保存対象になる重要なデータ）
        auto& trans = obj->GetTransform();
        ImGui::DragFloat3("Position", &trans.position.x, 0.1f);
        ImGui::DragFloat3("Rotation", &trans.rotation.x, 0.1f);
        ImGui::DragFloat3("Scale", &trans.scale.x, 0.01f);

        // マテリアルのカラーの編集
        ImGui::Separator();
        auto& color = obj->GetColor();
        ImGui::ColorEdit4("Material Color", &color.x);

        // 単色化虹色か切り替えるチェックボックス
        ImGui::Checkbox("Use Solid Color", &obj->GetUseSolidColor());
    }
    else
    {
        ImGui::Text("The object is not selected..."); // オブジェクトが選択されていない場合のメッセージ
    }

    if (app->m_selectedObjectIndex != -1)
    {
        if (ImGui::Button("Make Prefab"))
        {
            std::string path = app->m_gameObjects[app->m_selectedObjectIndex]->GetName() + ".pfb";
            app->SavePrefab(app->m_selectedObjectIndex, path); // プレハブとして保存
        }
    }






    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(400, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_FirstUseEver);
    ImGui::Begin("Engine Tuner"); // ウィンドウ名
    ImGui::Text("Transform"); // テキストの表示
    ImGui::ColorEdit4("BackgroundColor", app->m_backgroundColor);
    ImGui::End();

    // FPSなどの統計情報(オーバーレイ表示)
    ImGui::SetNextWindowPos(ImVec2(10, 570));
    ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();
}