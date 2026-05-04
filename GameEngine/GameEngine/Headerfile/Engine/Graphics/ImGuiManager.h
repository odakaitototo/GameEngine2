#pragma once
#include <windows.h>
#include <d3d11.h>

class ImGuiManager
{
public:
	ImGuiManager();
	~ImGuiManager();

	void Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
	void Begin(); // フレームの開始
	void End();   // フレームの終了
	void Terminate(); // クリーンアップ

};