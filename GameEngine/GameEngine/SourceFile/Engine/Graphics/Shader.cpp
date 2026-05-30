#include "Engine/Graphics/Shader.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib") // シェーダーをコンパイルする機能を使うためのライブラリ

bool Shader::Load(ID3D11Device* device, const std::wstring& fileName)
{
	ComPtr<ID3DBlob> vsBlob; // コンパイルされた頂点シェーダーの一時保存箱
	ComPtr<ID3DBlob> psBlob; // コンパイルされたピクセルシェーダーの一時保存箱
	ComPtr<ID3DBlob> errorBlob;

	// 頂点シェーダーのコンパイル
	HRESULT hr = D3DCompileFromFile(fileName.c_str(), nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	if (FAILED(hr))return false;

	// ピクセルシェーダーのコンパイル
	hr = D3DCompileFromFile(fileName.c_str(), nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &errorBlob);
	if (FAILED(hr)) return false;

	// Deviceを使って、GPU用の本番データを作成
	device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
	device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);

	// インプットレイアウトの作成
	// C++側の「struct Vertex」の並び順（座標->色）を、GPUに正確に伝えるための目次
	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA,0}, // 12という数字は、座標（float3 = 4バイト*3 = 12バイト）の次から色（COLOR）が始まるという意味
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// サンプラーの作成
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // きれいに拡大拡小する
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; // はみ出したらリピート
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&sampDesc, &m_samplerState);

	HRESULT hrLayout = device->CreateInputLayout(ied, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
	
	if (FAILED(hrLayout))
	{
		OutputDebugStringA("===Error: CreateInputLayout Failed! C++ and HLSL mismatch! ===\n");
		return false;
	}

	/*device->CreateInputLayout(ied, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);*/

	return true;
}

void Shader::Bind(ID3D11DeviceContext* context)
{
	// Contextに、このシェーダーと目次（レイアウト）を使うように命令する
	context->IASetInputLayout(m_inputLayout.Get());
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	// ピクセルシェイダーにサンプラーをセット
	context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
}

