#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>

using Microsoft::WRL::ComPtr;

class Shader
{
public:
	Shader() = default;
	~Shader() = default;

	// HLSLファイルを読み込んで、コンパイルしてGPUに登録する関数
	bool Load(ID3D11Device* device, const std::wstring& filename);

	// Contextに「今からこのシェイダーで塗れ」と指示する関数
	void Bind(ID3D11DeviceContext* context);

private:

	ComPtr<ID3D11VertexShader> m_vertexShader; // 頂点シェイダーのデータ
	ComPtr<ID3D11PixelShader> m_pixelShader; // ピクセルシェイダーのデータ
	ComPtr<ID3D11InputLayout> m_inputLayout; // C++の構造体とHLSLの橋渡し役

	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState; // サンプラー
};
