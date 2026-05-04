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

            // 操作パネルの描画
			ImGui::Begin("Inspector"); // ウィンドウ名
			ImGui::Text("Hello, Imgui!"); // テキストの表示
            ImGui::End();

            // 描画開始
			m_dx.BeginScene(0.1f, 0.1f, 0.3f, 1.0f); // 背景色を設定（例: 濃い青）
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
LRESULT CALLBACK Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}