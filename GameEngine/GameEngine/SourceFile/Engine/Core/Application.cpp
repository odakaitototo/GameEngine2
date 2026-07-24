#include "Engine/Core/Application.h"
#include "Engine/Scene/GameObject.h"
#include "imgui.h"

////////////////////////////////////////////////////////-----メモ----////////////////////////////////////////////////////////////////////
//
// shared_ptr(ポインタ)でMeshを管理する理由：メモリの節約：GAmeObject自体に頂点データを持たすと
// 　　　　　　　　　　　　　　　　　　　　　　　　　　　　オブジェクトを増やすたびにGPUのメモリが消費されるから。
// 　　　　　　　　　　　　　　　　　　　　　　　　　　　　ポインタにすれば実態は一つで済むためメモリが枯渇することはない。
// 　　　　　　　　　　　　　　　　　　　　：高速な描画：同じメッシュを使いまわすことで、
// 　　　　　　　　　　　　　　　　　　　　　　　　　　　「同じメッシュを使うオブジェクトをまとめて描画（インスタンシング）」
// 　　　　　　　　　　　　　　　　　　　　　　　　　　　という最適化が可能
// 
// addToList：再起関数（自分自身を呼び出す関数）
// 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static Application * g_app = nullptr; // WindowProcからApplication本体を操作するためのグローバル変数

Application::Application() : m_hWnd(nullptr), m_hInstance(nullptr) {}

Application::~Application() {}

// 初期化処理
bool Application::Initialize(HINSTANCE hInstance, int width, int height)
{

    g_app = this; // 自分自身を登録

    SetProcessDPIAware();
    m_hInstance = hInstance;

    // ウィンドウクラスの設定
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      L"RescueTraceEngine", NULL };
    RegisterClassEx(&wc);

    // ウィンドウの作成
    m_hWnd = CreateWindow(wc.lpszClassName, L"Project: Rescue Trace - Engine v0.1",
        WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
        100, 100, width, height, NULL, NULL, wc.hInstance, NULL);

    if (!m_hWnd) return false;

    DragAcceptFiles(m_hWnd, TRUE); // Windowsに「ドラッグ＆ドロップ」の許可をする


    // 3. DirectXの初期化
    if (!m_dx.Initialize(m_hWnd, width, height))
    {
        return false; // 初期化に失敗したら起動しない
    }

    // Scene窓口の裏紙リソースを初期サイズで作っておく
    if (!m_dx.CreateSceneResources(width, height))
    {
        return false;
    }

    // 最初のサイズを記憶させておく
    m_sceneWidth = (float)width;
    m_sceneHeight = (float)height;

    ShowWindow(m_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hWnd);

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    m_screenWidth = static_cast<float>(rc.right - rc.left);
    m_screenHeight = static_cast<float>(rc.bottom - rc.top);

    // 起動時に1度だけ、カメラの横縦比を教える
    m_camera.SetAspect((float)width, (float)height);


    // Imguiの初期化（DirectXのデバイス等を渡す）
    m_imgui.Initialize(m_hWnd, m_dx.GetDevice(), m_dx.GetContext());

    // ドッキング機能を有効化する一文
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    m_commonMesh = std::make_shared<Mesh>();
    std::vector<Vertex> vertices =
    {
        // 前面 (Z = -0.5)
        {{ -0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }}, // 0
        {{  0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }}, // 1
        {{ -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }}, // 2
        {{  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }}, // 3

        // 背面 (Z = 0.5)
        {{  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }}, // 4
        {{ -0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }}, // 5
        {{  0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }}, // 6
        {{ -0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }}, // 7

        // 上面 (Y = 0.5)
        {{ -0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }}, // 8
        {{  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }}, // 9
        {{ -0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }}, // 10
        {{  0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }}, // 11

        // 下面 (Y = -0.5)
        {{ -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }}, // 12
        {{  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }}, // 13
        {{ -0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }}, // 14
        {{  0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }}, // 15

        // 左面 (X = -0.5)
        {{ -0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }}, // 16
        {{ -0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }}, // 17
        {{ -0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }}, // 18
        {{ -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }}, // 19

        // 右面 (X = 0.5)
        {{  0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }}, // 20
        {{  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }}, // 21
        {{  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }}, // 22
        {{  0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }}  // 23
    };

    // インデックスデータ（頂点を結ぶ計算）
    std::vector<UINT> indices =
    {
        0, 1, 2,  2, 1, 3,       // 前面
        4, 5, 6,  6, 5, 7,       // 背面
        8, 9, 10, 10, 9, 11,     // 上面
        12, 13, 14, 14, 13, 15,  // 下面
        16, 17, 18, 18, 17, 19,  // 左面
        20, 21, 22, 22, 21, 23   // 右面
    };
    m_commonMesh = std::make_shared<Mesh>();

    m_commonMesh->Create(m_dx.GetDevice(), vertices, indices);

    // グリッドの頂点データ生成
    std::vector<Vertex> gridVertices;
    std::vector<UINT> gridIndices;
    int gridSize = 20;
    UINT gridIndex = 0;
    for (int i = -gridSize; i <= gridSize; i++)
    {
        gridVertices.push_back({ {(float)i, 0.0f, (float)-gridSize }, { 0.4f, 0.4f, 0.4f, 1.0f }, {0.0f,0.0f} });
        gridVertices.push_back({ { (float)i, 0.0f, (float)gridSize },  { 0.4f, 0.4f, 0.4f, 1.0f }, {0.0f,0.0f} });
        gridIndices.push_back(gridIndex++);
        gridIndices.push_back(gridIndex++);

    }

    for (int i = -gridSize; i <= gridSize; i++) {
        gridVertices.push_back({ { (float)-gridSize, 0.0f, (float)i }, { 0.4f, 0.4f, 0.4f, 1.0f }, { 0.0f,0.0f } });
        gridVertices.push_back({ { (float)gridSize, 0.0f, (float)i },  { 0.4f, 0.4f, 0.4f, 1.0f }, {0.0f,0.0f} });
        gridIndices.push_back(gridIndex++);
        gridIndices.push_back(gridIndex++);
    }

    m_gridMesh = std::make_shared<Mesh>();
    m_gridMesh->Create(m_dx.GetDevice(), gridVertices, gridIndices);
    m_gridMesh->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    // シェーダーの生成と読み込み
    m_shader = std::make_unique<Shader>();
    // L"..." とすることで、Wchar_t型 (std::wstring用)の文字列にします
    if (!m_shader->Load(m_dx.GetDevice(), L"ShaderFile/SimpleShader.hlsl"))
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

    // バッファを生成（初期データは後でマイフレーム送るのでnullptr）
    m_dx.GetDevice()->CreateBuffer(&cbDesc, nullptr, &m_pConstantBuffer);



    // レンズの設定
    float aspectRatio = m_screenWidth / m_screenHeight;
    float fovAngle = DirectX::XMConvertToRadians(60.0f);
    m_camera.SetPerspective(fovAngle, aspectRatio, 0.3f, 1000.0f);



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

        //毎フレーム、カメラの行列を最新状態に更新する
        m_camera.Update();

        // 親子関係（ワールド行列）の更新
        for (auto& obj : m_gameObjects)
        {
            if (obj->GetParent() == nullptr)
            {
                obj->UpdateTransform();
            }
        }

        m_dx.BeginSceneTexture(m_sceneWidth, m_sceneHeight, 0.2f, 0.f, 0.2f, 1.0f);

        // 3D空間を描画
        m_renderer.Render(this);

        // メイン画面への描画フェーズ
        // 描画先をメイン画面に戻し画面をクリアする
        // エディタ自体の背景色
        m_dx.BeginScene(0.2f, 0.2f, 0.2f, 1.0f);

        m_editorUI.Draw(this); // 「this」は、Application自身が「私のポインタを使ってね」 


        //// 描画開始
        //m_dx.BeginScene(m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2], m_backgroundColor[3]);
        //m_renderer.Render(this); // 描画処理


        // ImGuiをDirectXの上に重ねて描画
        m_imgui.End();
        m_dx.EndScene(); // 描画終了

    }
}

void Application::Terminate()
{
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

        // ファイルがドロップされたときの合図を取得する
    case WM_DROPFILES:
        if (g_app)
        {
            // wParamに「落とされたファイルの情報」があるので渡す
            g_app->OnDropFiles((HDROP)wParam);
        }
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
    for (int i = 0; i < m_gameObjects.size(); i++)
    {

        json j = m_gameObjects[i]->ToJson(); // 全オブジェクトのデータをジェイソン化

        if (j.is_object())
        {

            // 親が誰かを探して(出席番号)Jsonに追記する
            int parentIndex = -1;
            GameObject* parent = m_gameObjects[i]->GetParent();
            if (parent != nullptr)
            {
                // 全オブジェクトの中から親を探して番号を特定する
                for (int p = 0; p < m_gameObjects.size(); p++)
                {
                    if (m_gameObjects[p].get() == parent)
                    {
                        parentIndex = p;
                        break;
                    }
                }
            }
            j["ParentIndex"] = parentIndex; // 親の番号を保存

            root.push_back(j);
        }
    }

    std::ofstream ofs(filename);
    if (ofs)
    {
        ofs << root.dump(4);
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
    try {
        ifs >> root; // 構文エラーがあってもソフトを落とさない
    }
    catch (...) {
        return;
    }

    // 一番外側が配列じゃない（古い形式・壊れたデータ）場合は中止
    if (!root.is_array()) return;

    m_gameObjects.clear(); //現在のシーンをリセットする

    // 全員をとりあえず生成する
    for (const auto& j : root)
    {
        // オブジェクトの形じゃない異常なデータは飛ばす
        if (!j.is_object()) continue;

        auto obj = std::make_shared<GameObject>("");
        obj->FromJson(j, m_dx.GetDevice()); // テクスチャなどを復元
        obj->SetMesh(m_commonMesh); // 読み込んだオブジェクトに「形」を与える
        m_gameObjects.push_back(obj);
    }

    // 2. 保存しておいた出席番を見て、親子関係を結び直す
    for (int i = 0; i < root.size(); i++)
    {

        if (root[i].is_object() && root[i].contains("ParentIndex"))
        {

            if (root[i]["ParentIndex"].is_number())
            {
                int parentIndex = root[i]["ParentIndex"];
                if (parentIndex >= 0 && parentIndex < m_gameObjects.size())
                {
                    m_gameObjects[i]->SetParent(m_gameObjects[parentIndex].get());
                }
            }
        }
    }
}

// GameObjectで生成しているJson形式のオブジェクト情報を確認し親子関係を調べ登録する
void Application::SavePrefab(int index, const std::string& filename)
{
    if (index < 0 || index >= m_gameObjects.size())
    {
        return;
    }

    json root = json::array(); // 配列

    std::vector<GameObject*> prefabObjects; // 親と、そのすべての子供を配列にまとめる

    // 再帰的に子供をリストに追加する(親を0番として->子供1番->子供2番)
    std::function<void(GameObject*)> addToList = [&](GameObject* obj)
        {
            prefabObjects.push_back(obj);
            for (auto* child : obj->GetChildren())
            {
                addToList(child);
            }
        };

    // 選択されたオブジェクトを起点に、全階層をリストアップ
    addToList(m_gameObjects[index].get());

    // まとめたリストをJSON配列にする
    for (int i = 0; i < prefabObjects.size(); i++)
    {
        json j = prefabObjects[i]->ToJson();

        // 親の「プレハブ内での出席番号」を探す
        int parentIndex = -1;
        GameObject* parent = prefabObjects[i]->GetParent();
        if (parent != nullptr)
        {
            for (int p = 0; p < prefabObjects.size(); p++)
            {
                if (prefabObjects[p] == parent)
                {
                    parentIndex = p;
                    break;
                }
            }
        }
        j["ParentIndex"] = parentIndex; // 親の番号を保存
        root.push_back(j);
    }

    // オブジェクト名をファイル名にして出力する
    std::ofstream ofs(filename);
    if (ofs)
    {
        ofs << root.dump(4); // まとめたリストを保存する
    }
}

void Application::InstantiatePrefab(const std::string& filename)
{
    std::ifstream ifs(filename);
    if (!ifs)
    {

        return; // ファイルが見つからなければ何もしない
    }

    json root;

    // ファイルからデータをrootに読み込む
    try
    {
        ifs >> root;
    }
    catch (...)
    {
        return;
    }

    // 今のシーンのオブジェクト数を記憶しておく(番号のずれを直すため)
    // カメラなどの既存のオブジェクトにも番号が降られているから
    int startIndex = (int)m_gameObjects.size();

    for (const auto& j : root)
    {
        if (!j.is_object())
        {
            continue;
        }

        auto obj = std::make_shared<GameObject>("");
        obj->FromJson(j, m_dx.GetDevice());
        obj->SetMesh(m_commonMesh); // 形をセット

        // 大元の親(プレハブの一つ目のデータ)だけに(Clone)を付ける
        if (m_gameObjects.size() == startIndex)
        {
            obj->SetName(obj->GetName() + "(Clone)");
        }

        // シーンに配置
        m_gameObjects.push_back(obj);
    }

    // 出席番号を見て、親子関係を結び直す
    for (int i = 0; i < root.size(); i++)
    {
        if (root[i]["ParentIndex"].is_number())
        {
            int parentIdx = root[i]["ParentIndex"];
            if (parentIdx >= 0)
            {
                // プレハブ内の出席番号　+　追加前のシーンオブジェクト数　＝　実際の出席番号
                int actualParentIndex = startIndex + parentIdx;
                int actualChildIndex = startIndex + i;

                // 親子関係の復元
                m_gameObjects[actualChildIndex]->SetParent(m_gameObjects[actualParentIndex].get());
            }
        }
    }


}

// オブジェクトの複製
void Application::ObujectDuplication()
{

    // 何も選択されていなければ処理しない
    if (m_selectedObjectIndex == -1 || m_selectedObjectIndex >= m_gameObjects.size())
    {
        return;
    }

    // 再帰的に　階層まるごと複製する関数
    std::function<std::shared_ptr<GameObject>(GameObject*, GameObject*)> deepCopy = [&](GameObject* original, GameObject* parent)->std::shared_ptr<GameObject>
        {
            // 自分自信をクローン
            auto cloneObj = original->Clone();
            cloneObj->SetName(original->GetName() + "_Copy"); // 名前にコピーを付ける

            // 新しいオブジェクトリストに追加
            m_gameObjects.push_back(cloneObj);

            // 親を設定
            if (parent != nullptr)
            {
                cloneObj->SetParent(parent);
            }

            // 子供たちも全員クローンする
            for (auto* child : original->GetChildren())
            {
                deepCopy(child, cloneObj.get());
            }

            return cloneObj;
        };
    // 選択中のオブジェクトを親としてコピー開始
    auto originalObj = m_gameObjects[m_selectedObjectIndex].get();
    auto rootClone = deepCopy(originalObj, originalObj->GetParent());

    // 完全に重ならないように大本の親だけ位置をずらす
    rootClone->GetTransform().position.x += 2.0f;

    // クローンされた大本の親を選択状態にする
    for (int i = 0; i < m_gameObjects.size(); ++i)
    {
        if (m_gameObjects[i] == rootClone)
        {
            m_selectedObjectIndex = i;
            break;
        }
    }



}



void Application::ResizeScene(float width, float height)
{
    // 同じサイズなら何もしない
    if (width == m_sceneWidth && height == m_sceneHeight)
    {
        return;
    }

    // 新しいサイズを記録
    m_sceneWidth = width;
    m_sceneHeight = height;

    // DirectXのテクスチャを新しいサイズで作り直す
    m_dx.CreateSceneResources((int)width, (int)height);

    // カメラのアスペクト比を更新んして、歪みを消す
    m_camera.SetAspect(width, height);
}

// マウスピッキングの実装
void Application::PickObject(float mouseX, float mouseY, float viewWidth, float viewHeight)
{
    // カメラの行列を取得
    DirectX::XMMATRIX view = m_camera.GetViewMatrix();
    DirectX::XMMATRIX proj = m_camera.GetProjectionMatrix();
    DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

    // 2Dのモニター座標から、3D空間へのRayを作る
    DirectX::XMVECTOR rayOrigin = DirectX::XMVector3Unproject(
        DirectX::XMVectorSet(mouseX, mouseY, 0.0f, 0.0f), // 画面の手前
        0, 0, viewWidth, viewHeight, 0.0f, 1.0f,
        proj, view, world
    );

    DirectX::XMVECTOR farPoint = DirectX::XMVector3Unproject(
        DirectX::XMVectorSet(mouseX, mouseY, 1.0f, 0.0f), // 画面の奥
        0, 0, viewWidth, viewHeight, 0.0f, 1.0f,
        proj, view, world
    );

    DirectX::XMVECTOR rayDir = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(farPoint, rayOrigin)); // レーザーの向きを計算

    // 各オブジェクトのレーザーが当たっているかを調べる
    float minDistance = 1000000.0f; //　一番手前にあるレーザーを見つけるよう
    int hitIndex = -1; // 当たったオブジェクトの番号

    for (int i = 0; i < m_gameObjects.size(); i++)
    {
        auto& t = m_gameObjects[i]->GetTransform();

        // 上で計算済みのワールド行列をそのまま読み込む
        DirectX::XMMATRIX objWorld = DirectX::XMLoadFloat4x4(&t.worldMatrix);


        // レーザーを「オブジェクトの視点（ローカル空間）」に逆変換する
        DirectX::XMVECTOR det;
        DirectX::XMMATRIX invWorld = DirectX::XMMatrixInverse(&det, objWorld);

        DirectX::XMVECTOR localOrigin = DirectX::XMVector3TransformCoord(rayOrigin, invWorld);
        DirectX::XMVECTOR localDir = DirectX::XMVector3TransformNormal(rayDir, invWorld);
        localDir = DirectX::XMVector3Normalize(localDir);

        // サイコロの当たり判定箱
        DirectX::BoundingBox box(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f));

        float distance = 0.0f;

        if (box.Intersects(localOrigin, localDir, distance))
        {
            // カメラから一番近い（手前にある）ものを選択する
            if (distance < minDistance)
            {
                minDistance = distance;
                hitIndex = i;
            }
        }
    }

    // 3. 当たっていたら選択状態にする（空振りなら -1 になって選択解除される）
    m_selectedObjectIndex = hitIndex;

}


void Application::RecordUndo()
{
    if (m_selectedObjectIndex == -1)
    {
        return; // 何も選択されていなければ無視
    }

    auto& t = m_gameObjects[m_selectedObjectIndex]->GetTransform();

    // 現在の数値をメモ帳に書き写す
    UndoRecord rec;
    rec.objectIndex = m_selectedObjectIndex;
    rec.px = t.position.x; rec.py = t.position.y; rec.pz = t.position.z;
    rec.rx = t.rotation.x; rec.ry = t.rotation.y; rec.rz = t.rotation.z;
    rec.sx = t.scale.x;  rec.sy = t.scale.y;  rec.sz = t.scale.z;

    // 履歴リストの最後に追加する
    m_undoStack.push_back(rec);
}

// Ctrl+Zが押されたときに呼び出し、過去の状態を復元する
void Application::ExecuteUndo()
{
    if (m_undoStack.empty())
    {
        return; // 履歴がなければ何もしない
    }

    // 履歴リストの一番最後（最新の過去）を取り出す
    UndoRecord rec = m_undoStack.back();
    m_undoStack.pop_back(); // 取り出した履歴はリストから消す

    // インデックスが安全か確認してから復元する
    if (rec.objectIndex >= 0 && rec.objectIndex < m_gameObjects.size())
    {
        auto& t = m_gameObjects[rec.objectIndex]->GetTransform();
        t.position = { rec.px, rec.py, rec.pz };
        t.rotation = { rec.rx, rec.rz, rec.rz };
        t.scale = { rec.sx, rec.sy, rec.sz };

        // 復元したオブジェクトを選択状態にする
        m_selectedObjectIndex = rec.objectIndex;
    }

}



// ドラグ＆ドロップでテクスチャーを張り付ける処理
void Application::OnDropFiles(HDROP hDrop)
{
    char filePath[MAX_PATH];

    // 落とされたファイルの内、1番目のファイルパスを読み取る
    if (DragQueryFileA(hDrop, 0, filePath, MAX_PATH))
    {
        // オブジェクトが選択されているか確認
        if (m_selectedObjectIndex != -1)
        {
            // 新しいテクスチャを作って読み込む
            auto newTexture = std::make_shared<Texture>();

            // ドラッグ＆ドロップなら絶対パス（filePath）が確実に入る
            if (newTexture->Load(m_dx.GetDevice(), filePath))
            {
                // 選択中のオブジェクトに画像をセットして、テクスチャモードをONにする
                m_gameObjects[m_selectedObjectIndex]->SetTexture(newTexture);
                m_gameObjects[m_selectedObjectIndex]->GetUseSolidColor() = false;

                OutputDebugStringA("マテリアルを適応しました");
            }
        }
    }
    // Windowsに「ファイルの受け取り処理が終わりました」と報告
    DragFinish(hDrop);
}

////////////////////////////////
// 
// モード切替
//
////////////////////////////////


// Playモード時の処理
void Application::StartPlayMode()
{
    // モードが既にPlayだったらreturnを返す
    if (m_engineMode == EngineMode::Play)
    {
        return;
    }

    // Playモードに切り替える前にシーン全体をJson配列としてメモリにバックアップする
    m_sceneBackup = json::array();

    for (int i = 0; i < m_gameObjects.size(); i++)
    {
        json j = m_gameObjects[i]->ToJson(); // i番目のオブジェクトデータをjに保存

        int parentIndex = -1;
        GameObject* parent = m_gameObjects[i]->GetParent();
        if (parent != nullptr)
        {
            for (int p = 0; p < m_gameObjects.size(); p++)
            {
                if (m_gameObjects[p].get() == parent)
                {
                    parentIndex = p;
                    break;
                }
            }
            
        }
        j["parentIndex"] = parentIndex;
        m_sceneBackup.push_back(j);
       
    }

    // モードをPlayに切り替える
    m_engineMode = EngineMode::Play;
    m_selectedObjectIndex = -1; // 選択状態を解除

    OutputDebugStringA("▶　Play Mode Started\n");
}


void Application::StopPlayMode()
{
    // モードがすでにSditorモードならreturnを返す
    if (m_engineMode == EngineMode::Editor)
    {
        return;
    }

    // Play中にかかった変更を元に戻す
    m_gameObjects.clear();

    // バックアップしておいたJsonからPlay直前の状態を復元する
    for (const auto& j : m_sceneBackup)
    {
        if (!j.is_object())
        {
            continue;
        }

        auto obj = std::make_shared<GameObject>("");
        obj->FromJson(j, m_dx.GetDevice());
        obj->SetMesh(m_commonMesh);
        m_gameObjects.push_back(obj);
    }

    // 親子関係の結び直し
    for (int i = 0; i < m_sceneBackup.size(); i++)
    {
        if (m_sceneBackup[i].is_object() && m_sceneBackup[i].contains("parentIndex"))
        {

            if (m_sceneBackup[i]["parentIndex"].is_number())
            {
                int parentIndex = m_sceneBackup[i]["parentIndex"];

                if (parentIndex >= 0 && parentIndex < m_gameObjects.size())
                {
                    auto savedPos = m_gameObjects[i]->GetTransform().position;
                    auto savedRot = m_gameObjects[i]->GetTransform().rotation;
                    auto saveScl = m_gameObjects[i]->GetTransform().scale;

                    m_gameObjects[i]->SetParent(m_gameObjects[parentIndex].get());

                    m_gameObjects[i]->GetTransform().position = savedPos;
                    m_gameObjects[i]->GetTransform().rotation = savedRot;
                    m_gameObjects[i]->GetTransform().scale = saveScl;
                    m_gameObjects[i]->UpdateTransform();
                }
            }
        }
    }

    // モードをEditorに切り替える
    m_engineMode = EngineMode::Editor;
    m_sceneBackup.clear(); // メモリ句を開放

    OutputDebugStringA("■ Editor Mode Restored\n");
}
