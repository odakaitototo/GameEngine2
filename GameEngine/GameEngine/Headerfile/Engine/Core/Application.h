#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>

#include "Engine/Graphics/DirectXManager.h"
#include "Engine/Graphics/ImGuiManager.h"
#include "Engine/Scene/GameObject.h"

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

public: // 保存と読み込みの関数
    void SaveScene(const std::string& filename);
	void LoadScene(const std::string& filename);

public: // プレハブの保存と読み込みの関数
    void SavePrefab(int index, const std::string& filename);
    void InstantiatePrefab(const std::string& filename);

private:
    // Windowsのメッセージを処理する関数
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    DirectXManager m_dx; // DirectX管理クラス

    HWND m_hWnd;
    HINSTANCE m_hInstance;

private:
    // ImGui関連
    ImGuiManager m_imgui;
    float m_backgroundColor[4] = { 0.1f, 0.1f, 0.3f, 1.0f }; // 背景色を保持する変数

private:
    //ゲームオブジェクトのリスト（スマートポインターで完全に管理）
    std::vector<std::shared_ptr<GameObject>> m_gameObjects;

	// 現在選択されているゲームオブジェクトインデックス（Inspectorで編集するため）
    int m_selectedObjectIndex = -1;  
};