#include "Engine/Core/Application.h"
#include <imgui.h>

Application::Application() : m_hWnd(nullptr), m_hInstance(nullptr) {}

Application::~Application() {}

bool Application::Initialize(HINSTANCE hInstance, int width, int height) {
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

    // Imguiの初期化（DirectXのデバイス等を渡す）
    m_imgui.Initialize(m_hWnd, m_dx.GetDevice(), m_dx.GetContext());

    return true;
}

void Application::Run() {
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
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
                m_gameObjects.push_back(std::make_shared<GameObject>(name));
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

            // ImGuiをDirectXの上に重ねて描画
            m_imgui.End();

            m_dx.EndScene(); // 描画終了
        }
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
    std::ofstream ofs(filename);
    if (!ofs)
    {
		return; // ファイルが開けなかったら保存しない
    }

	for (const auto& obj : m_gameObjects)// ゲームオブジェクトの数だけループ
    {
		auto& t = obj->GetTransform(); // Transform情報を取得
		// 名前、PosX、PosY、PosZ、RotX、RotY、RotZ、ScaleX、ScaleY、ScaleZの順で保存
        ofs << obj -> GetName() << " "
            << t.position.x << " " << t.position.y << " " << t.position.z << " "
            << t.rotation.x << " " << t.rotation.y << " " << t.rotation.z << " "
			<< t.scale.x    << " " << t.scale.y    << " " << t.scale.z    << "\n";
    }
    std::cout << "Scene Saved:" << filename << std::endl;
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
        return; // ファイルが開けなかったら読み込まない
    }

    m_gameObjects.clear(); // 既存のオブジェクトを全て削除
    m_selectedObjectIndex = -1; // 選択もリセット

    std::string name;
    float posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ;

    while (ifs >> name >> posX >> posY >> posZ >> rotX >> rotY >> rotZ >> scaleX >> scaleY >> scaleZ)
    {
        auto obj = std::make_shared<GameObject>(name); // オブジェクトを作成
        auto& t = obj->GetTransform(); // Transform情報を取得して設定
        t.position = { posX, posY, posZ };
        t.rotation = { rotX, rotY, rotZ };
        t.scale = { scaleX, scaleY, scaleZ };
        m_gameObjects.push_back(obj); // リストに追加
    }
    std::cout << "Scene Loaded:" << filename << std::endl;
}

void Application::SavePrefab(int index, const std::string& filename)
{
    if (index < 0 || index >= m_gameObjects.size())
    {
        return;
    }

    std::ofstream ofs(filename);
    auto& obj = m_gameObjects[index];
    auto& t = obj->GetTransform();

	// 1つ分のデータだけを書き出す（名前、PosX、PosY、PosZ、RotX、RotY、RotZ、ScaleX、ScaleY、ScaleZの順）
    ofs << obj->GetName() << " "
        << t.position.x << " " << t.position.y << " " << t.position.z << " "
        << t.rotation.x << " " << t.rotation.y << " " << t.rotation.z << " "
        << t.scale.x    << " " << t.scale.y    << " " << t.scale.z    << "\n";
	std::cout << "Prefab Saved:" << filename << std::endl;
}

void Application::InstantiatePrefab(const std::string& filename)
{
    std::ifstream ifs(filename);
    if (!ifs)
    {
        return;
    }

    std::string name;
    float posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ;

    if (ifs >> name >> posX >> posY >> posZ >> rotX >> rotY >> rotZ >> scaleX >> scaleY  >> scaleZ)
    {
        // 新しいオブジェクトとしてシーンに追加（名前の重複を避けるために"(Clone)"を付与）
        auto obj = std::make_shared<GameObject>(name + "(Clone)");
        auto& t = obj->GetTransform();
        t.position = { posX, posY, posZ };
        t.rotation = { rotX, rotY, rotZ };
        t.scale = { scaleX, scaleY, scaleZ };
        m_gameObjects.push_back(obj);
    }
    else
    {
		std::cout << "Failed to load prefab:" << filename << std::endl;
    }
}
