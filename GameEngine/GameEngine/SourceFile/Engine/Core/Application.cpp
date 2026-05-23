#include "Engine/Core/Application.h"
#include <imgui.h>

////////////////////////////////////////////////////////-----メモ----////////////////////////////////////////////////////////////////////
//
// shared_ptr(ポインタ)でMeshを管理する理由：メモリの節約：GAmeObject自体に頂点データを持たすと
// 　　　　　　　　　　　　　　　　　　　　　　　　　　　　オブジェクトを増やすたびにGPUのメモリが消費されるから。
// 　　　　　　　　　　　　　　　　　　　　　　　　　　　　ポインタにすれば実態は一つで済むためメモリが枯渇することはない。
// 　　　　　　　　　　　　　　　　　　　　：高速な描画：同じメッシュを使いまわすことで、
// 　　　　　　　　　　　　　　　　　　　　　　　　　　　「同じメッシュを使うオブジェクトをまとめて描画（インスタンシング）」
// 　　　　　　　　　　　　　　　　　　　　　　　　　　　という最適化が可能
// 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


Application::Application() : m_hWnd(nullptr), m_hInstance(nullptr) {}

Application::~Application() {}

// 初期化処理
bool Application::Initialize(HINSTANCE hInstance, int width, int height)
{
    m_hInstance = hInstance;

    // ウィンドウクラスの設定
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      L"RescueTraceEngine", NULL };
    RegisterClassEx(&wc);

    // ウィンドウの作成
    m_hWnd = CreateWindow(wc.lpszClassName, L"Project: Rescue Trace - Engine v0.1",
        WS_OVERLAPPEDWINDOW, 100, 100, width, height, NULL, NULL, wc.hInstance, NULL);

    if (!m_hWnd) return false;

    // 3. DirectXの初期化
    if (!m_dx.Initialize(m_hWnd, width, height)) 
    {
        return false; // 初期化に失敗したら起動しない
    }

    ShowWindow(m_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hWnd);

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    m_screenWidth = static_cast<float>(rc.right - rc.left);
    m_screenHeight = static_cast<float>(rc.bottom - rc.top);

    // Imguiの初期化（DirectXのデバイス等を渡す）
    m_imgui.Initialize(m_hWnd, m_dx.GetDevice(), m_dx.GetContext());

   m_commonMesh = std::make_shared<Mesh>();
    std::vector<Vertex> vertices =
    {
        // 前面 (Z = -0.5)
        {{ -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }}, // 0: 左上 (赤)
        {{  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }}, // 1: 右上 (緑)
        {{ -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }}, // 2: 左下 (青)
        {{  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f, 1.0f }}, // 3: 右下 (黄)

        // 背面 (Z = 0.5)
        {{ -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }}, // 4: 左上 (マゼンタ)
        {{  0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 1.0f, 1.0f }}, // 5: 右上 (シアン)
        {{ -0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }}, // 6: 左下 (白)
        {{  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 0.0f, 1.0f }}  // 7: 右下 (黒)
    };

    // インっデックスデータ（頂点を結ぶ計算）
    std::vector<UINT> indices =
    {
        0, 1, 2,  2, 1, 3, // 前面
        5, 4, 7,  7, 4, 6, // 背面
        4, 0, 6,  6, 0, 2, // 左面
        1, 5, 3,  3, 5, 7, // 右面
        4, 5, 0,  0, 5, 1, // 上面
        2, 3, 6,  6, 3, 7  // 下面
    };
    m_commonMesh = std::make_shared<Mesh>();

    m_commonMesh ->Create(m_dx.GetDevice(), vertices, indices);

    // シェーダーの生成と読み込み
    m_shader = std::make_unique<Shader>();
    // L"..." とすることで、Wchar_t型 (std::wstring用)の文字列にします
    if (!m_shader->Load(m_dx.GetDevice(), L"/HAL/Game/DX/GameEngine/GameEngine/GameEngine/ShaderFile/SimpleShader.hlsl"))
    {
        return false; // シェーダーのコンパイルに失敗したら起動しない(安全設計)
    }

    // 定数バッファの作成
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(ConstantBufferTransform); // サイズ
    cbDesc.Usage = D3D11_USAGE_DYNAMIC; // 動的用に書き換える
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // 
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPUからの書き込みを許可
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = 0;

     // ばっふぁを生成（初期データは後でマイフレーム送るのでnullptr）
    m_dx.GetDevice()->CreateBuffer(&cbDesc, nullptr, &m_pConstantBuffer);

    return true;
}

void Application::Run() 
{
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                break;
            }

        }
        if (msg.message == WM_QUIT)
        {
            break;
        }
            // ImGuiのフレーム開始
            m_imgui.Begin();


            // Hierarchyウィンドウ（左側に配置）
            ImGui::SetNextWindowPos(ImVec2(400, 0), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
            ImGui::Begin("Hierarchy");

            if (ImGui::Button("Save Scene"))
            {
                SaveScene("scene.txt"); // シーンを保存
			}
            if (ImGui::Button("Load Scene"))
            {
                LoadScene("scene.txt"); // シーンを読み込み
			}
            ImGui::Separator();
            // オブジェクト作成ボタン
            if (ImGui::Button("Create Empty"))
            {
                // 新しいオブジェクトを作成してリストに追加
                std::string name = "GameObject" + std::to_string(m_gameObjects.size());
                auto newObj = std::make_shared<GameObject>(name);

                newObj->SetMesh(m_commonMesh); // 新しく作ったオブジェクトに、共通メッシュの住所を教える

                m_gameObjects.push_back(newObj);
            }
            ImGui::Separator();
            if (ImGui::Button("Instantiate Prefab"))
            {
                InstantiatePrefab("GameObject0.pfb"); // プレハブを読み込んでシーンに追加
            }
            ImGui::Separator();
            ImGui::Text("Scene Objects");
            ImGui::Separator(); // 区切るための線
            
            for (int i = 0; i < m_gameObjects.size(); i++)
            {
                bool isSelected = (m_selectedObjectIndex == i);
                if (ImGui::Selectable(m_gameObjects[i]->GetName().c_str(), isSelected))
                {
                    m_selectedObjectIndex = i; // 選択したオブジェクトの番号を保存
                }
            }

            ImGui::End();

            // Inspectorウィンドウ（右側に配置）
            ImGui::SetNextWindowPos(ImVec2(600, 0),ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
			ImGui::Begin("Inspector"); // ウィンドウ名
            if (m_selectedObjectIndex != -1 && m_selectedObjectIndex < m_gameObjects.size())
            {
                auto& obj = m_gameObjects[m_selectedObjectIndex];

                // 名前編集
                char buf[128];
                strcpy_s(buf, obj->GetName().c_str());
                if (ImGui::InputText("Name", buf, sizeof(buf)))
                {
                    obj -> SetName(buf);
                }
                ImGui::Separator();

                // Transform編集（ここが保存対象になる重要なデータ）
                auto& trans = obj->GetTransform();
                ImGui::DragFloat3("Position", &trans.position.x, 0.1f);
                ImGui::DragFloat3("Rotation", &trans.rotation.x, 0.1f);
				ImGui::DragFloat3("Scale",    &trans.scale.x,    0.01f);
            }
            else
            {
                ImGui::Text("The object is not selected..."); // オブジェクトが選択されていない場合のメッセージ
            }

            if (m_selectedObjectIndex != -1)
            {
                if (ImGui::Button("Make Prefab"))
                {
                    std::string path = m_gameObjects[m_selectedObjectIndex]->GetName() + ".pfb";
                    SavePrefab(m_selectedObjectIndex, path); // プレハブとして保存
                }
            }
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(400, 0), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_FirstUseEver);
            ImGui::Begin("Engine Tuner"); // ウィンドウ名
            ImGui::Text("Transform"); // テキストの表示
            ImGui::ColorEdit4("BackgroundColor", m_backgroundColor);
            ImGui::End();

            // FPSなどの統計情報(オーバーレイ表示)
            ImGui::SetNextWindowPos(ImVec2(10, 570));
            ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::End();

            // 描画開始
            m_dx.BeginScene(m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2], m_backgroundColor[3]);
            // ここで今後のUpdateやDrawを呼び出します

            m_shader->Bind(m_dx.GetContext()); // シェイダーを使うためにGPUに指示する

            // カメラの行列を計算する
            // View行列（カメラの位置と向き）
            // Z軸の手前(-5.0f)に少し浮かせた(Y:2.0f)位置から、原点(0,0,0)を見下ろすカメラ
            DirectX::XMVECTOR eyePosition = DirectX::XMVectorSet(0.0f, 1.0f, -10.0f, 0.0f);
            DirectX::XMVECTOR focusPoint = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 真っ直ぐ奥を見る
            DirectX::XMVECTOR upDirection = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);


            // Projection行列(レンズの設定)
            // 画角60度、アスペクト比(画面比率)、近くの限界0.1f、遠くの限界100.0ｆ
            float fovAngle = DirectX::XMConvertToRadians(60.0f);
            float aspectRatio = m_screenWidth / m_screenHeight; // 実際のウィンドウの幅 / 高さに合わせる
            DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fovAngle, aspectRatio, 0.1f, 100.0f);

            // シーンに存在する全てのゲームオブジェクトをループ描画する
            for (int i = 0; i < m_gameObjects.size(); i++)
            {
                // Transformを取得
                auto& t = m_gameObjects[i]->GetTransform();

                // スケール・回転・平行移動の行数を作成
                DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z);

                float radX = DirectX::XMConvertToRadians(t.rotation.x);
                float radY = DirectX::XMConvertToRadians(t.rotation.y);
                float radZ = DirectX::XMConvertToRadians(t.rotation.z);
                DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(radX, radY, radZ);
                DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(t.position.x, t.position.y, t.position.z);

                // 3つの行数を掛け合わせてワールド行列を完成させる
                DirectX::XMMATRIX worldMatrix = scale * rotation * translation;

                // 定数バッファの構造体にデータを詰める
                ConstantBufferTransform cbData;

                // HLSLは行列の読み込み方法がC++と逆なので、転置して送る
                cbData.worldMatrix = DirectX::XMMatrixTranspose(worldMatrix);

                // ViewとProjectionも転置して詰める
                cbData.viewMatrix = DirectX::XMMatrixTranspose(viewMatrix);
                cbData.projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);

                D3D11_MAPPED_SUBRESOURCE mappedResource;

                // D3D11_MAP_WRITE_DISCARDが重要
                // 「前の箱はGPUが使っているかもしれないから、古いのは破棄して、新しい箱を用意して」という命令
                if (SUCCEEDED(m_dx.GetContext()->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
                {
                    // もらった新しい箱(pData)に、行列データを直接流し込む
                    memcpy(mappedResource.pData, &cbData, sizeof(ConstantBufferTransform));

                    // 箱を閉じる
                    m_dx.GetContext()->Unmap(m_pConstantBuffer.Get(), 0);

                }



                // 「0番目のスロット(b0)」ここの定数バッファをセットする
                m_dx.GetContext()->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

                // データの準備完了　描画
                m_gameObjects[i]->Draw(m_dx.GetContext());


            }

            // ImGuiをDirectXの上に重ねて描画
            m_imgui.End();

            m_dx.EndScene(); // 描画終了
        
    }
}

void Application::Terminate() {
    UnregisterClass(L"RescueTraceEngine", m_hInstance);
}

// Windows OSからの通知（閉じるボタンなど）を処理する
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT mag, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) // ImGuiかWindowsのイベントかを判断
        return true;

    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}


//////////////////////////////////////
// 
// シーンの保存（ファイルに書き出す）
//
//////////////////////////////////////
void Application::SaveScene(const std::string& filename)
{
    json root = json::array(); // シーン全体を配列する
    for (const auto& obj : m_gameObjects)
    {
        root.push_back(obj->ToJson());
    }

    std::ofstream ofs(filename);
    if (ofs)
    {
        ofs << root.dump(4); // インデックス4で保存
    }
}

//////////////////////////////////////
// 
// シーンの読み込み（ファイルから読み込む）
//
//////////////////////////////////////
void Application::LoadScene(const std::string& filename)
{
    std::ifstream ifs(filename);
    if (!ifs)
    {
        return;
    }

    json root;
    ifs >> root;

    m_gameObjects.clear();
    for (const auto& j : root)
    {
        auto obj = std::make_shared<GameObject>("");
        obj->FromJson(j);

        obj->SetMesh(m_commonMesh); // 読み込んだオブジェクトに「形」を与える
        m_gameObjects.push_back(obj);
    }
}

void Application::SavePrefab(int index, const std::string& filename)
{
    if (index < 0 || index >= m_gameObjects.size())
    {
        return;
    }

    std::ofstream ofs(filename);
    if (ofs)
        {
        ofs << m_gameObjects[index]->ToJson().dump(4);
        }
}

void Application::InstantiatePrefab(const std::string& filename)
{
    std::ifstream ifs(filename);
    if (!ifs)
    {
        return;
    }

        json j;
        ifs >> j; // JSONとして解析

        auto obj = std::make_shared<GameObject>("");
        obj -> FromJson(j);
        obj->SetName(obj->GetName() + "(Clone)");

        obj->SetMesh(m_commonMesh); // クローンしたオブジェクトに「形」を与える
        m_gameObjects.push_back(obj);
    
}


 