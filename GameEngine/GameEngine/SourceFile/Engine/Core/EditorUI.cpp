#include "Engine/Core/EditorUI.h"
#include "Engine/Core/Application.h"
#include "imgui.h"
#include "ImGuizmo.h"

#include <string>
#include <functional>
#include <filesystem>

namespace fs = std::filesystem;

void EditorUI::Draw(Application* app)
{
    // 画面全体をはめ込み用ボードにする
    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    // Sceneウィンドウ（ビューボート）
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(640, 400), ImGuiCond_FirstUseEver);

    // 空白をゼロにし、スクロールを完全にできなくする。画面ずれを防ぐ
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGuiWindowFlags sceneFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("Scene", nullptr, sceneFlags);
    ImGui::PopStyleVar(); // スタイルを元に戻す

    // Ctrl+Zが押されたときに呼び出し、ひとつ前の操作まで戻る
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
    {
        app->ExecuteUndo();
    }
    // Ctrl+Dが押されたときに呼び出し、選択中のオブジェクトを複製する
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false))
        {
        app->ObujectDuplication();
        }

    // Deleteキーでオブジェクトの消去
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && app->m_selectedObjectIndex != -1)
    {
        // 選択中のオブジェクトをリストから消し去る
        app->m_gameObjects.erase(app->m_gameObjects.begin() + app->m_selectedObjectIndex);
        app->m_selectedObjectIndex = -1; // 選択解除
        app->m_undoStack.clear();
    }


    // 現在のSceneの中の使えるスペースを取得
    ImVec2 sceneWindowSize = ImGui::GetContentRegionAvail();

    // サイズが0よりおおきければ、Appplicationにサイズ変更を伝える
    if (sceneWindowSize.x > 0.0f && sceneWindowSize.y > 0.0f)
    {
        app->ResizeScene(sceneWindowSize.x, sceneWindowSize.y);
    }

    // 取得したスペースの大きさに合わせて、3Dを描画したテクスチャを画像として表示
    ImGui::Image((void*)app->m_dx.GetSceneSRV(), sceneWindowSize);

    // Scene画面へのドラッグ＆ドロップ受付
    if (ImGui::BeginDragDropTarget())
    {

        // プレハブのパスを受け取る
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
        {
            const char* path = (const char*)payload->Data;

            // ファイルの拡張子が.pfbだった時だけプレハブとして出現させる
            std::string filePath(path);
            if (filePath.find(".pfb") != std::string::npos)
            {
                app->InstantiatePrefab(filePath);
            }
        }
        ImGui::EndDragDropTarget();
        
    }

    // ギズモを描画するために、画像の座標とサイズを記憶しておく
    ImVec2 imagePos = ImGui::GetItemRectMin();
    ImVec2 imageSize = ImGui::GetItemRectSize();

    // 画像がクリックされたかどうかの状態だけを保存しておく
    bool isImageHovered = ImGui::IsItemHovered();
    bool isMouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

   

    // エディタカメラの操作
    if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        // マウスの移動量を取得して,カメラを回転させる
        ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
        app->m_camera.Rotate(mouseDelta.y * 0.2f, mouseDelta.x * 0.2f);

        // WASDキーでカメラを移動させる
        float moveSpeed = 0.1f;
        float dRight = 0.0f, dUp = 0.0f, dForward = 0.0f;

        if (ImGui::IsKeyDown(ImGuiKey_W))
        {
            dForward += moveSpeed; // 前
        }

        if (ImGui::IsKeyDown(ImGuiKey_S))
        {
            dForward -= moveSpeed; // 後ろ
        }

        if (ImGui::IsKeyDown(ImGuiKey_D))
        {
            dRight += moveSpeed; // 右
        }

        if (ImGui::IsKeyDown(ImGuiKey_A))
        {
            dRight -= moveSpeed; // 左
        }

        if (ImGui::IsKeyDown(ImGuiKey_Q))
        {
            dUp += moveSpeed;  // 上
        }

        if (ImGui::IsKeyDown(ImGuiKey_E))
        {
            dUp -= moveSpeed; // 下
        }

        // Shiftキーを押している間は早くなる
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift))
        {
            dRight *= 3.0f;
            dForward *= 3.0f;
        }

        app->m_camera.Move(dRight, dUp, dForward);
    }

    // ImGuizmoによるオブジェクト直感操作システム
    ImGuizmo::BeginFrame();

    // ギズモサイズを変更する場所
    ImGuizmo::Style& gizmoStyle = ImGuizmo::GetStyle();
    gizmoStyle.TranslationLineThickness = 6.0f;  // 移動の線の太さ
    gizmoStyle.TranslationLineArrowSize = 12.0f; // 移動の矢印の大きさ
    gizmoStyle.RotationLineThickness = 6.0f;  // 回転の線の太さ
    gizmoStyle.RotationOuterLineThickness = 5.0f;  // 回転の外側の線の太さ
    gizmoStyle.ScaleLineThickness = 6.0f;  // 拡縮の線の太さ
    gizmoStyle.ScaleLineCircleSize = 12.0f; // 拡縮の先端の大きさ

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(imagePos.x, imagePos.y, imageSize.x, imageSize.y);

    if (app->m_selectedObjectIndex != -1 && app->m_selectedObjectIndex < app->m_gameObjects.size())
    {
        // オブジェクト本体も取得しておく
        auto selectedObj = app->m_gameObjects[app->m_selectedObjectIndex];
        auto& trans = selectedObj->GetTransform();

        // カメラ行列（レンズのデータ）の取得
        DirectX::XMFLOAT4X4 view, proj;
        DirectX::XMStoreFloat4x4(&view, app->m_camera.GetViewMatrix());
        DirectX::XMStoreFloat4x4(&proj, app->m_camera.GetProjectionMatrix());

        // ローカル座標の合成をやめ、すでに計算済みの「ワールド行列（絶対座標）」をそのままギズモに渡す！
        DirectX::XMFLOAT4X4 worldFloat = trans.worldMatrix;
        float* objectMatrix = &worldFloat.m[0][0];

        // キーボードで操作モードを切り替え（右クリックでのカメラ移動中以外）
        static ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) mCurrentGizmoOperation = ImGuizmo::TRANSLATE; // Wキーで移動
            if (ImGui::IsKeyPressed(ImGuiKey_E)) mCurrentGizmoOperation = ImGuizmo::ROTATE;    // Eキーで回転
            if (ImGui::IsKeyPressed(ImGuiKey_R)) mCurrentGizmoOperation = ImGuizmo::SCALE;     // Rキーで拡縮
        }

        // ギズモを画面に描画し、マウスドラッグの操作を受け付ける
        ImGuizmo::Manipulate(&view.m[0][0], &proj.m[0][0], mCurrentGizmoOperation, ImGuizmo::LOCAL, objectMatrix);

        // もしギズモがマウスで操作されたら
        if (ImGuizmo::IsUsing())
        {
            // ギズモが動かした後の「新しいワールド行列」
            DirectX::XMMATRIX newWorldMat = DirectX::XMLoadFloat4x4(&worldFloat);

            // 親がいる場合は「逆行列」を掛けて、ローカル座標に戻してから保存する
            DirectX::XMMATRIX newLocalMat;
            if (selectedObj->GetParent() != nullptr)
            {
                DirectX::XMVECTOR det;
                DirectX::XMMATRIX parentWorld = DirectX::XMLoadFloat4x4(&selectedObj->GetParent()->GetTransform().worldMatrix);
                DirectX::XMMATRIX invParentWorld = DirectX::XMMatrixInverse(&det, parentWorld);

                newLocalMat = newWorldMat * invParentWorld; // ワールド座標からローカル座標への変換
            }
            else
            {
                newLocalMat = newWorldMat; // 親がいなければそのまま
            }

            // 計算した新しいローカル行列から、位置・回転・スケールの数値を抜き出して Transform に上書きする
            DirectX::XMFLOAT4X4 localFloat;
            DirectX::XMStoreFloat4x4(&localFloat, newLocalMat);

            float resultTrans[3], resultRot[3], resultScale[3];
            ImGuizmo::DecomposeMatrixToComponents(&localFloat.m[0][0], resultTrans, resultRot, resultScale);

            trans.position = { resultTrans[0], resultTrans[1], resultTrans[2] };
            trans.rotation = { resultRot[0], resultRot[1], resultRot[2] };
            trans.scale = { resultScale[0], resultScale[1], resultScale[2] };

            // 行列を再計算して、子オブジェクトたちにも動きを伝える
            selectedObj->UpdateTransform();
        }
    }

    // 当たり判定をギズモの後に移動させる
    if (isImageHovered && isMouseClicked && !ImGuizmo::IsOver())
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        float localMouseX = mousePos.x - imagePos.x;
        float localMouseY = mousePos.y - imagePos.y;

        // 当たり判定を実行する
        app->PickObject(localMouseX, localMouseY, sceneWindowSize.x, sceneWindowSize.y);
    }
    ImGui::End();


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
    static char prefabLoadBuf[128] = "Yuka.pfb";
    ImGui::InputText("Prefab File", prefabLoadBuf, sizeof(prefabLoadBuf));

    // 上で入力された名前（prefabLoadBuf）のファイルを読み込む！
    if (ImGui::Button("Instantiate Prefab"))
    {
        app->InstantiatePrefab(prefabLoadBuf);
    }
    ImGui::Separator(); // 区切るための線
   
    std::function<void(GameObject*, int)> drawNode = [&](GameObject* obj, int index)
    {
           
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
            if (app->m_selectedObjectIndex == index)
            {
                flags |= ImGuiTreeNodeFlags_Selected; // 選択中なら色を変える
            }

            
            if (obj->GetChildren().empty())
            {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }

            // クローン対策
            std::string displayLabel = obj->GetName() + "_" + std::to_string(index);


            bool isOpen = ImGui::TreeNodeEx((void*)(intptr_t)index, flags, "%s", displayLabel.c_str());

            if (ImGui::IsItemClicked())
            {
                app->m_selectedObjectIndex = index;
            }

            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("GAMEOBJECT", &index, sizeof(int));
                ImGui::Text("Move %s", obj->GetName().c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT"))
                {
                    int droppedIndex = *(const int*)payload->Data;
                    GameObject* droppedObj = app->m_gameObjects[droppedIndex].get();

                    if (droppedObj != obj)
                    {
                        droppedObj->SetParent(obj);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (isOpen)
            {
                // 子供がいる場合は順番に描画（再帰呼び出し）
                for (auto* child : obj->GetChildren())
                {
                    int childIndex = -1;
                    for (int i = 0; i < app->m_gameObjects.size(); ++i)
                    {
                        if (app->m_gameObjects[i].get() == child)
                        {
                            childIndex = i;
                            break;
                        }
                    }
                    if (childIndex != -1)
                    {
                        drawNode(child, childIndex);
                    }
                }

                ImGui::TreePop();
            }
    };

    // 親がいない単体のオブジェクト表示
    for (int i = 0; i < app->m_gameObjects.size(); ++i)
    {
        if (app->m_gameObjects[i]->GetParent() == nullptr)
        {
            drawNode(app->m_gameObjects[i].get(), i);
        }
    }

    // ヒエラルキーの何もないところにドロップするとおやこかんけいを解除できる
    ImGui::Dummy(ImGui::GetContentRegionAvail());
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT"))
        {
            int droppedIndex = *(const int*)payload->Data;
            app->m_gameObjects[droppedIndex]->SetParent(nullptr); // 親をなっしにする
        }

        // Hierarchyの余白にもプレハブを読み込めるようにする
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
        {
            const char* path = (const char*)payload->Data;
            std::string filePath(path);
            if (filePath.find(".pfb") != std::string::npos)
            {
                app->InstantiatePrefab(filePath);
            }
        }



        ImGui::EndDragDropTarget();
    
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
        if (ImGui::IsItemActivated())
        {
            app->RecordUndo();
        }
        ImGui::DragFloat3("Rotation", &trans.rotation.x, 0.1f);
        if (ImGui::IsItemActivated())
        {
            app->RecordUndo();
        }
        ImGui::DragFloat3("Scale", &trans.scale.x, 0.01f);
        if (ImGui::IsItemActivated())
        {
            app->RecordUndo();
        }

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

    // 矢印キーによるオブジェクトの固定値移動
    if (app->m_selectedObjectIndex != -1)
    {
        ImGui::Separator();
        ImGui::Text("SnapMovement (Arrow Keys)");

        // 移動する幅
        float snapValue = 1.0f;

        // 選択中のオブジェクトの Transform を取得
        auto& trans = app->m_gameObjects[app->m_selectedObjectIndex]->GetTransform();

        // UIの入力と、3D空間のショートカット入力を分離する
        // ImGui::GetID().WantTextInputは「今、テキストボックスに文字を打ち込んでいるか？」を判定
        if (!ImGui::GetIO().WantTextInput)
        {
            static bool prevRight = false, prevLeft = false, prevUp = false, prevDown = false;

            // 今の瞬間のキー状態をWindows　APIから直接取得 (ImGuiの横取りを無視)
            bool currRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
            bool currLeft = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
            bool currUp = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
            bool currDown = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;




            // ImGuiが矢印キーが押された瞬間を検知したら、座標を足し引きする
            if (ImGui::IsKeyPressed(ImGuiKey_L)) // 右矢印
            {
                app->RecordUndo();
                trans.position.x += snapValue;

            }

            if (ImGui::IsKeyPressed(ImGuiKey_J)) // 左矢印
            {
                app->RecordUndo();
                trans.position.x -= snapValue;

            }

            if (ImGui::IsKeyPressed(ImGuiKey_I)) // 上矢印
            {
                app->RecordUndo();
                trans.position.z += snapValue;

            }

            if (ImGui::IsKeyPressed(ImGuiKey_K)) // 下矢印
            {
                app->RecordUndo();
                trans.position.z -= snapValue;

            }

            if (ImGui::IsKeyPressed(ImGuiKey_U)) // 上矢印
            {
                app->RecordUndo();
                trans.position.y += snapValue;

            }

            if (ImGui::IsKeyPressed(ImGuiKey_O)) // 下矢印
            {
                app->RecordUndo();
                trans.position.y -= snapValue;

            }

            // 状態を更新
            prevRight = currRight;
            prevLeft  = currLeft;
            prevUp    = currUp;
            prevDown = currDown;

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

    ///////////////////////////////////////////////
    //
    //プロジェクトウィンドウ(画面下部ウィンドウ)
    //
    ////////////////////////////////////////////////
    ImGui::SetNextWindowPos(ImVec2(10, 600), ImGuiCond_FirstUseEver); // 画面の下の位置に配置
    ImGui::SetNextWindowSize(ImVec2(780, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Project Browser");

    // 右クリックメニューの作成
    if (ImGui::BeginPopupContextWindow("ProjectContext"))
    {
        if (ImGui::MenuItem("Create New Prefab"))
        {
            // 空のプレハブファイルを作る処理
            std::ofstream ofs("NewPrefab.pfb");
            ofs << "{}";
            ofs.close();
        }

        if (ImGui::MenuItem("Create New Script(HLSL)"))
        {
            // 空のシェーダーファイルを作る処理
            std::ofstream ofs("NewShader.hlsl");
            ofs << "// New Shader";
            ofs.close();
        }
       
        ImGui::EndPopup();
    }

    ImGui::Text("Current Directory: ./ (Right-click to create files)");
    ImGui::Separator();

    // ファイルを横に並べるための準備
    float cellSize = 100.0f;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth) / cellSize;
    if (columnCount < 1)
    {
        columnCount = 1;
    }

    ImGui::Columns(columnCount, 0, false);

    // プロジェクトの実行フォルダの中身をループで取得する
    std::string currentPath = ".";
    for (const auto& entry : fs::directory_iterator(currentPath))
    {
        auto path = entry.path();
        std::string filename  = path.filename().string();
        std::string extension = path.extension().string();

        // プレハブ(pfb)とシェーダー・スクリプト(hlsl, txt)、画像(png)だけを表示する
        if (extension == ".pfb" || extension == ".hlsl" || extension == ".txt" || extension == ".png" || extension == ".jpg")
        {
            ImGui::PushID(filename.c_str());

            // アイコン代わりのボタン
            ImGui::Button(filename.c_str(), ImVec2(90, 90));

            // ドラッグ＆ドロップ
            if (ImGui::BeginDragDropSource())
            {
                // ファイルパを文字列として詰める
                std::string itemPath = path.string();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath.c_str(), itemPath.size() + 1);
                ImGui::Text("Load %s", filename.c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::TextWrapped("%s", filename.c_str());
            ImGui::NextColumn();
            ImGui::PopID();
        }
    }

    ImGui::Columns(1);
    ImGui::End();



    /////////////////////////////
    //
    // モード切り替え
    // 
    /////////////////////////////

    ImGui::Begin("Game State");

    if (app->GetEngineMode() == EngineMode::Editor)
    {
        // エディタモードの時は緑色の ▶ PLAY ボタンを表示
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        if (ImGui::Button("PLAY", ImVec2(100, 30)))
        {
            app->StartPlayMode();
        }
        ImGui::PopStyleColor();
        
    }
    else
    {
        // エディタモード時は赤色の ■ STOP ボタンを表示
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("STOP", ImVec2(100, 30)))
        {
            app->StopPlayMode();
        }
        ImGui::PopStyleColor();
    }
    ImGui::End();
}