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
            ImGui::Text("Scene Objects");
            ImGui::Separator(); // 区切るための線
            if (ImGui::Selectable("Main Camera", true)) // メインのカメラ
            {

            }
            if (ImGui::Selectable("Directional Light")) // 太陽の光のようなもの
            {

            }
            if (ImGui::Selectable("Triangle")) // 三角形オブジェクト
            {

            }
            ImGui::End();

            // Inspectorウィンドウ（右側に配置）
            ImGui::SetNextWindowPos(ImVec2(600, 0),ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
			ImGui::Begin("Inspector"); // ウィンドウ名
			ImGui::Text("Transform"); // テキストの表示
            static float pos[3] = { 0,0,0 };
            ImGui::DragFloat3("Position", pos, 0.1f);
            ImGui::Separator();
            ImGui::ColorEdit4("Background", m_backgroundColor);
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