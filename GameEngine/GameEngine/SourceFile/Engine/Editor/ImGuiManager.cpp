#include "Engine/Editor/ImGuiManager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// コンストラクタ
ImGuiManager::ImGuiManager() {}

// デストラクタ（ここで「クラス名::~クラス名」と正しく記述します）
ImGuiManager::~ImGuiManager() {
    Terminate();
}

void ImGuiManager::Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* context) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Unity風のダークテーマを設定
    ImGui::StyleColorsDark();

    // バックエンド（Win32とDirectX 11）の初期化
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(device, context);
}

void ImGuiManager::Begin() {
    // フレームの開始
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::End() {
    // 描画実行
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManager::Terminate() {
    // 解放処理
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}