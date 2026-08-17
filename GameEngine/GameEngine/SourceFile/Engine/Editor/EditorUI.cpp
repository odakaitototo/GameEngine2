#include "Engine/Editor/EditorUI.h"
#include "Engine/Core/Application.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "Engine/Component/ColliderBase.h"
#include "Engine/Component/AABBColliderComponent.h"
#include "Engine/Component/OBBColliderComponent.h"
#include "Engine/Component/RigidbodyComponent.h"
#include "Engine/Scene/SceneManager.h"
#include "Engine/Component/MeshRendererComponent.h"


#include <string>
#include <functional>
#include <filesystem>
#include <vector>

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

        if (app->GetEngineMode() == EngineMode::Play)
        {
            // プレイモード中　右クリックドラッグでプレイ用のカメラを動かす
            app->GetGameCamera().Rotate(mouseDelta.x, -mouseDelta.y);
        }
        else
        {
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

    }

    if (ImGui::IsWindowHovered() && app->GetEngineMode() == EngineMode::Play)
    {
        float wheelDelta = ImGui::GetIO().MouseWheel;
        if (wheelDelta != 0.0f)
        {
            // ホイールを回した分だけカメラ距離を変更する
            app->GetGameCamera().Zoom(wheelDelta);
        }
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


    ImGui::Separator(); // 見た目を区切る横線
    ImGui::Text("Scene Management");

    // 文字を記憶するためのバッファを用意する
    static char sceneNameBuffer[128] = "Stage1";

    // 文字が入力できるテキストボックス
    ImGui::InputText("Scene Name", sceneNameBuffer, sizeof(sceneNameBuffer));

    if (ImGui::Button("Save Scene"))
    {
        std::string filename = std::string(sceneNameBuffer) + ".json";
        SceneManager::SaveScene(app, filename);// シーンを保存
    }

    ImGui::SameLine(); // 改行しないようにする

    if (ImGui::Button("Load Scene"))
    {
        std::string filename = std::string(sceneNameBuffer) + ".json";
        SceneManager::LoadScene(app, filename);// シーンを読み込み
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

    // 最上部にMainCameraを常時表示
    ImGuiTreeNodeFlags camFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (app->m_isCameraSelected)
    {
        camFlags |= ImGuiTreeNodeFlags_Selected; // 選択中ならハイライトする
    }

    ImGui::TreeNodeEx("GameCameraNode", camFlags, "Main Camera (OrbitCamera)");

    if (ImGui::IsItemClicked())
    {
        app->m_selectedObjectIndex = -1; // 普通のオブジェクトの選択を解除
        app->m_isCameraSelected = true; // カメラを選択状態にする
    }

    ImGui::Separator();


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
                app->m_isCameraSelected = false; // 通常のオブジェクトを選んだらカメラ選択を解除
            }

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

    // カメラが選択されていたら、カメラ調節パネルを表示
    if (app->m_isCameraSelected)
    {
        ImGui::Text("Main Camera Settings");
        ImGui::Separator();

        auto& cam = app->GetGameCamera();

        // プレビュー機能のスイッチ
        ImGui::Checkbox("Preview Game Camera", &app->m_previewGameCamera);
        ImGui::Separator();

        ImGui::Text("Target & Position");
        ImGui::DragFloat3("Target Pos", &cam.GetTarget().x, 0.1f);

        if (ImGui::Button("ResetDefaultView"))
        {
            cam.GetTarget() = { 0.0f,0.0f,0.0f };
            cam.GetDistance() = 15.0f;
            cam.GetYaw() = 45.0f;
            cam.GetPitch() = 30.0f;
        }

        ImGui::Separator();
        ImGui::DragFloat("Distance", &cam.GetDistance(), 0.5f, cam.GetMinDistance(), cam.GetMaxDistance());
        ImGui::DragFloat("Yaw (Angle X)", &cam.GetYaw(), 1.0f);
        ImGui::DragFloat("Pitch (Angle Y)", &cam.GetPitch(), 0.5f, cam.GetMinPitch(), cam.GetMaxPitch());

        ImGui::Separator();
        ImGui::Text("Limits & Speed");
        ImGui::DragFloatRange2("Pitch Limit", &cam.GetMinPitch(), &cam.GetMaxPitch(), 1.0f, -89.0f, 89.0f);
        ImGui::DragFloatRange2("Distance Limit", &cam.GetMinDistance(), &cam.GetMaxDistance(), 1.0f, 0.1f, 500.0f);
        ImGui::DragFloat("Rotate Speed", &cam.GetRotateSpeed(), 0.01f, 0.01f, 2.0f);
        ImGui::DragFloat("Zoom Speed", &cam.GetZoomSpeed(), 0.1f, 0.1f, 10.0f);
    }
    else if (app->m_selectedObjectIndex != -1 && app->m_selectedObjectIndex < app->m_gameObjects.size())
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

        ImGui::Separator();
        ImGui::Text("Texture Settings");

        // ドロップを受け付けるためのダミーボタン（枠）を描画
        ImGui::Button("Drop Texture Here (.png / .jpg)", ImVec2(-1, 40));

        // もしこの枠に何かがドロップされたら
        if (ImGui::BeginDragDropTarget())
        {
            // Project Browser から運ばれてきたデータ（CONTENT_BROWSER_ITEM）を受け取る
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::string filePath = (const char*)payload->Data;

                // 拡張子が画像（.png か .jpg）かどうかをチェック
                if (filePath.find(".png") != std::string::npos || filePath.find(".jpg") != std::string::npos)
                {
                    // オブジェクトが持つ MeshRenderer を取得する
                    auto render = obj->GetComponent<MeshRendererComponent>();
                    if (render != nullptr)
                    {
                        // 新しいテクスチャをメモリ上に作り、画像をロードして上書きする！
                        auto newTex = std::make_shared<Texture>();
                        if (newTex->Load(app->m_dx.GetDevice(), filePath))
                        {
                            render->texture = newTex;
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        auto collider = obj->GetComponent<ColliderBase>();
        if (collider != nullptr) // 当たり判定を持っていたらUIを表示！
        {
            ImGui::Separator();
            ImGui::Text("Collider Settings");

            // AABB（緑の箱）の場合のUI
            if (collider->GetColliderType() == ColliderType::AABB)
            {
                auto aabb = std::static_pointer_cast<AABBColliderComponent>(collider);

                ImGui::Text("Type: AABB (Fixed Box)");
                // 中心のズレ（オフセット）をいじる
                ImGui::DragFloat3("Center Offset", &aabb->localBoundingBox.Center.x, 0.05f);
                // 大きさをいじる（マイナスにならないように 0.0f ～ 100.0f に制限）
                ImGui::DragFloat3("Size (Extents)", &aabb->localBoundingBox.Extents.x, 0.05f, 0.0f, 100.0f);
            }
            // OBB（赤い箱）の場合のUI
            else if (collider->GetColliderType() == ColliderType::OBB)
            {
                auto obb = std::static_pointer_cast<OBBColliderComponent>(collider);

                ImGui::Text("Type: OBB (Rotatable Box)");
                // 中心のズレ（オフセット）をいじる
                ImGui::DragFloat3("Center Offset", &obb->localBoundingBox.Center.x, 0.05f);
                // 大きさをいじる（マイナスにならないように 0.0f ～ 100.0f に制限）
                ImGui::DragFloat3("Size (Extents)", &obb->localBoundingBox.Extents.x, 0.05f, 0.0f, 100.0f);
            }
        }
        ///////////////////////////////////
        // 
        // RigidbodyComponentのUI表示設定
        // 
        ///////////////////////////////////
        auto rb = obj->GetComponent<RigidbodyComponent>();
        if (rb != nullptr)
        {
            ImGui::Separator();
            ImGui::Text("Rigidbody");

            ImGui::Checkbox("Use Gravity", &rb->useGravity);

            // 数値をドラッグで変更できるUI
            ImGui::DragFloat("Gravity Scale", &rb->gravityScale, 0.1f - 10.0f, 10.0f);
            ImGui::DragFloat("Drag (Resistance)", &rb->drag, 0.1f, 0.0f, 10.0f);

            ImGui::Text("Freeze Position");
            ImGui::Checkbox("X##Pos", &rb->freezePosX); ImGui::SameLine();
            ImGui::Checkbox("Y##Pos", &rb->freezePosY); ImGui::SameLine();
            ImGui::Checkbox("Z##Pos", &rb->freezePosZ);
        }


        // コンポーネント追加UI 
        ImGui::Separator();
        ImGui::Text("Components");

        // AABB追加ボタン
        if (ImGui::Button("Add AABB Collider (Fixed)"))
        {
            // まだColliderを持っていなければ追加する
            if (!obj->GetComponent<ColliderBase>())
            {
                obj->AddComponent<AABBColliderComponent>();
            }
        }

        ImGui::SameLine(); // ボタンを横に並べる

        // OBB追加ボタン
        if (ImGui::Button("Add OBB Collider (Rotatable)"))
        {
            // まだColliderを持っていなければ追加する
            if (!obj->GetComponent<ColliderBase>())
            {
                obj->AddComponent<OBBColliderComponent>();
            }
        }

        // Rigidbodyの追加ボタン
        if (ImGui::Button("Add Rigidbody"))
        {
            if (!obj->GetComponent<RigidbodyComponent>())
            {
                obj->AddComponent<RigidbodyComponent>();
            }
        }
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
            bool currLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
            bool currUp = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
            bool currDown = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;




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
            prevLeft = currLeft;
            prevUp = currUp;
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

    ImGui::Separator();

    // フォルダ移動

    static fs::path currentDirectory = fs::current_path();
    if (currentDirectory != fs::current_path())
    {
        // もし現在の階層が一番上ではないときだけ戻るボタンを表示
        if (ImGui::Button("<- Back"))
        {
            currentDirectory = currentDirectory.parent_path();
        }

        ImGui::SameLine();
    }

    // ========================================================
    // 🌟 アップグレード1：検索バーとアイコンサイズ変更スライダー
    // ========================================================
    static char searchBuffer[128] = "";
    ImGui::SetNextItemWidth(200.0f); // 検索バーの幅
    ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer));

    ImGui::SameLine(); // 横に並べる

    static float thumbnailSize = 74.0f; // staticにして大きさを記憶させる
    ImGui::SetNextItemWidth(150.0f); // スライダーの幅
    ImGui::SliderFloat("Icon Size", &thumbnailSize, 32.0f, 128.0f); // 32〜128の間で自由に変更可能！

    ImGui::Separator();

    // グリッドの計算（thumbnailSize が変動するのでここで計算）
    float padding = 16.0f;
    float cellSize = thumbnailSize + padding;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    // フォルダとファイルを分けて整理（ソート）する
    std::vector<fs::directory_entry> folders;
    std::vector<fs::directory_entry> files;

    for (const auto& entry : fs::directory_iterator(currentDirectory))
    {
        if (entry.is_directory()) {
            folders.push_back(entry);
        }
        else {
            files.push_back(entry);
        }
    }

    std::vector<fs::directory_entry> sortedEntries;
    sortedEntries.insert(sortedEntries.end(), folders.begin(), folders.end());
    sortedEntries.insert(sortedEntries.end(), files.begin(), files.end());

    // 検索キーワードを string 型にしておく
    std::string searchStr = searchBuffer;

    for (const auto& entry : sortedEntries)
    {
        const auto& path = entry.path();
        std::string filename = path.filename().string();
        std::string extension = path.extension().string();

        // ========================================================
        // 🌟 アップグレード2：検索フィルター機能
        // ========================================================
        // 検索ボックスに文字が入っている場合、ファイル名に含まれていなければスキップ
        if (!searchStr.empty() && filename.find(searchStr) == std::string::npos)
        {
            continue;
        }

        bool isDrawn = false; // 🌟 描画したかどうかを記録するフラグ

        if (entry.is_directory())
        {
            ImGui::PushID(filename.c_str());
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.5f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.6f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.4f, 0.1f, 1.0f));

            if (ImGui::Button(" FOLDER ", ImVec2(thumbnailSize, thumbnailSize)))
            {
                currentDirectory /= path.filename();
            }
            ImGui::PopStyleColor(3);
            ImGui::TextWrapped("%s", filename.c_str());
            ImGui::PopID();

            isDrawn = true; // 描画した！
        }
        else if (extension == ".pfb" || extension == ".hlsl" || extension == ".txt" || extension == ".png" || extension == ".jpg" || extension == ".json" || extension == ".cpp" || extension == ".h")
        {
            ImGui::PushID(filename.c_str());
            ImVec4 btnColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
            ImVec4 hoverColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            ImVec4 activeColor = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
            std::string iconText = " FILE ";

            if (extension == ".pfb")
            {
                btnColor = ImVec4(0.15f, 0.35f, 0.55f, 1.0f); 
                hoverColor = ImVec4(0.2f, 0.45f, 0.65f, 1.0f); 
                activeColor = ImVec4(0.1f, 0.25f, 0.45f, 1.0f);
                iconText = "Prefab\n(.pfb)";
            }
            else if (extension == ".json")
            {
                btnColor = ImVec4(0.2f, 0.45f, 0.25f, 1.0f); 
                hoverColor = ImVec4(0.25f, 0.55f, 0.3f, 1.0f); 
                activeColor = ImVec4(0.15f, 0.35f, 0.2f, 1.0f);
                iconText = "Scene\n(.json)";
            }
            else if (extension == ".png" || extension == ".jpg")
            {
                btnColor = ImVec4(0.45f, 0.25f, 0.45f, 1.0f); 
                hoverColor = ImVec4(0.55f, 0.3f, 0.55f, 1.0f); 
                activeColor = ImVec4(0.35f, 0.15f, 0.35f, 1.0f);
                iconText = "Image\n(Tex)";
            }
            else if (extension == ".hlsl") 
            {
                btnColor = ImVec4(0.6f, 0.35f, 0.15f, 1.0f);
                hoverColor = ImVec4(0.7f, 0.45f, 0.2f, 1.0f); 
                activeColor = ImVec4(0.5f, 0.25f, 0.1f, 1.0f);
                iconText = "Shader\n(.hlsl)";
            }

            ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);

            if (ImGui::Button(iconText.c_str(), ImVec2(thumbnailSize, thumbnailSize)))
            {
                if (extension == ".json") SceneManager::LoadScene(app, path.string());
            }
            ImGui::PopStyleColor(3);

            if (ImGui::BeginDragDropSource())
            {
                std::string itemPath = path.string();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath.c_str(), itemPath.size() + 1);
                ImGui::Text("Load %s", filename.c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::TextWrapped("%s", filename.c_str());
            ImGui::PopID();

            isDrawn = true; // 描画した
        }

        
        if (isDrawn)
        {
            ImGui::NextColumn();
        }
    }

    ImGui::Columns(1);
        ImGui::End();



        /////////////////////////////
        //
        // モード切り替え
        // 
        /////////////////////////////

        // 画面上部に常時固定されるバーを作成
        if (ImGui::BeginMainMenuBar())
        {
            float menuBarWidth = ImGui::GetWindowWidth();
            float buttonWidth = 100.0f;

            // 画面全体の幅とボタンの幅から、真ん中の座標を計算してカーソルを移動
            ImGui::SetCursorPosX((menuBarWidth - buttonWidth) * 0.5f);

            if (app->GetEngineMode() == EngineMode::Editor)
            {
                // エディタモード時は緑色のPlayボタン
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));

                if (ImGui::Button(" PLAY ", ImVec2(buttonWidth, 0.0f)))
                {
                    app->StartPlayMode();
                }
                ImGui::PopStyleColor();
            }
            else
            {
                // プレイモードの時は赤色の STOP ボタンを表示
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));

                if (ImGui::Button(" STOP ", ImVec2(buttonWidth, 0.0f)))
                {
                    app->StopPlayMode();
                }
                ImGui::PopStyleColor();
            }
            ImGui::EndMainMenuBar();
        }
}
