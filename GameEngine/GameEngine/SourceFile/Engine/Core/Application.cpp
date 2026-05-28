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
        100,100, width, height, NULL, NULL, wc.hInstance, NULL);

    if (!m_hWnd) return false;

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

    // グリッドの頂点データ生成
    std::vector<Vertex> gridVertices;
    std::vector<UINT> gridIndices;
    int gridSize = 20;
    UINT gridIndex = 0;
    for (int i = -gridSize; i <= gridSize; i++)
    {
        gridVertices.push_back({ {(float)i, 0.0f, (float)-gridSize }, { 0.4f, 0.4f, 0.4f, 1.0f } });
        gridVertices.push_back({ { (float)i, 0.0f, (float)gridSize },  { 0.4f, 0.4f, 0.4f, 1.0f } });
        gridIndices.push_back(gridIndex++);
        gridIndices.push_back(gridIndex++);
    
    }

    for (int i = -gridSize; i <= gridSize; i++) {
        gridVertices.push_back({ { (float)-gridSize, 0.0f, (float)i }, { 0.4f, 0.4f, 0.4f, 1.0f } });
        gridVertices.push_back({ { (float)gridSize, 0.0f, (float)i },  { 0.4f, 0.4f, 0.4f, 1.0f } });
        gridIndices.push_back(gridIndex++);
        gridIndices.push_back(gridIndex++);
    }

    m_gridMesh = std::make_shared<Mesh>();
    m_gridMesh->Create(m_dx.GetDevice(), gridVertices, gridIndices);
    m_gridMesh->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

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

    // カメラの初期設定

    // 位置と向き
    m_camera.SetLookAt(
        DirectX::XMVectorSet(0.0f, 1.0f, -10.0f, 0.0f), // eyePosition（カメラの位置）
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), // focusPoint（見ている場所）
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) // upDirection（上方向）
    );

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

        // オブジェクトのワールド行列（配置データ）を計算
        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z);
        float radX = DirectX::XMConvertToRadians(t.rotation.x);
        float radY = DirectX::XMConvertToRadians(t.rotation.y);
        float radZ = DirectX::XMConvertToRadians(t.rotation.z);
        DirectX::XMMATRIX rot = DirectX::XMMatrixRotationRollPitchYaw(radX, radY, radZ);
        DirectX::XMMATRIX trans = DirectX::XMMatrixTranslation(t.position.x, t.position.y, t.position.z);
        DirectX::XMMATRIX objWorld = scale * rot * trans;

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




 