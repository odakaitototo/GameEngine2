#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>
#include <DirectXCollision.h> // 当たり判定を使うためのもの
#include "DirectXMath.h"

#include "Engine/Graphics/DirectXManager.h"
#include "Engine/Graphics/ImGuiManager.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/Camera.h"
#include "Engine/Core/EditorUI.h"
#include "Engine/Core/Renderer.h"
// GPUに送るデータの形式をHLSLと一致させる
struct ConstantBufferTransform
{
    DirectX::XMMATRIX worldMatrix; // 64バイト
    DirectX::XMMATRIX viewMatrix;
    DirectX::XMMATRIX projectionMatrix;

    DirectX::XMFLOAT4 materialColor; // 色情報

    int useSolidColor; // 4バイト（ONなら1　OFFなら0）
    DirectX::XMFLOAT3 dummy; // 12バイト（これで合計16バイト）
};

class Application {
    friend class EditorUI; // EditorUIクラスは、私のprivateデータにアクセスしてもよい、という一文
    friend class Renderer;
    
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

public: // ビューポート関係
    // Sceneのサイズが変った時に呼び出す関数
    void ResizeScene(float width, float height);

public: // オブジェクト関係

    void PickObject(float mouseX, float mouseY, float viewWidth, float viewHeight); // マウスクリックでオブジェクトを選択する関数


    

private:
    // Windowsのメッセージを処理する関数
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    float m_screenWidth; // Window画面の横幅の変数
    float m_screenHeight; // Window画面の縦幅の変数

    DirectXManager m_dx; // DirectX管理クラス

    HWND m_hWnd;
    HINSTANCE m_hInstance;

private:
    // ImGui関連
    ImGuiManager m_imgui;
    float m_backgroundColor[4] = { 0.1f, 0.1f, 0.3f, 1.0f }; // 背景色を保持する変数

    EditorUI m_editorUI;
    

private:
    //ゲームオブジェクトのリスト（スマートポインターで完全に管理）
    std::vector<std::shared_ptr<GameObject>> m_gameObjects;

	// 現在選択されているゲームオブジェクトインデックス（Inspectorで編集するため）
    int m_selectedObjectIndex = -1;  

private: // メッシュ関係
    std::shared_ptr<Mesh> m_commonMesh; // シーン全体で使いまわす、共通の三角形メッシュ

    std::shared_ptr<Mesh> m_gridMesh; // グリッド専用メッシュ

private: // シェーダー関係
    std::unique_ptr<Shader> m_shader; // シェーダーの管理用

private: //定数場hhぁの実態
    ComPtr<ID3D11Buffer> m_pConstantBuffer;

private: // カメラ関係
    Camera m_camera;
private:
    Renderer m_renderer;

private: // ビューポート関係
    // 現在のSceneのサイズを記憶しておくための変数
    float m_sceneWidth = 0.0f;
    float m_sceneHeight = 0.0f;
};