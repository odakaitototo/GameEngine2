#pragma once
#include <windows.h>
#include "Engine/Graphics/DirectXManager.h"

class Application {
public:
    Application();
    ~Application();

    // 初期化（ウィンドウ作成など）
    bool Initialize(HINSTANCE hInstance, int width, int height);

    // メインループ
    void Run();

    // 終了処理
    void Terminate();

private:
    // Windowsのメッセージを処理する関数
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    DirectXManager m_dx; // DirectX管理クラス

    HWND m_hWnd;
    HINSTANCE m_hInstance;
};