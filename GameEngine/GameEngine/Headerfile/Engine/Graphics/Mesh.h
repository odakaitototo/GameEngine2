#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

// 頂点1つのデータ構造
// SimpleShader.hlsliのstruct VS_INPUTと完全に一致させる
struct Vertex
{
	DirectX::XMFLOAT3 pos; // 位置（x,y,z）
	DirectX::XMFLOAT4 color; // 色 (r,g,b,a)

};

class Mesh
{
public:
	Mesh() = default;

	// Deviceを使って、頂点バッファを作成する
	// 立体を描画できるようにindicesを追加
	bool Create(ID3D11Device* device, const std::vector<Vertex>& vertices, const std::vector<UINT>& indices);

	// Contextに、このメッシュを描画する準備をさせる
	void Bind(ID3D11DeviceContext* context);

	// 実際に描画命令を出す
	void Draw(ID3D11DeviceContext* context);

public:
	void SetTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
	{
		m_topology = topology;
	}

private:
	ComPtr<ID3D11Buffer> m_pvertexBuffer; // GPU側のメモリに置かれた頂点データ
	UINT m_vertexCount = 0; // 頂点の数
	UINT m_stride = sizeof(Vertex); // 頂点1つ当たりのサイズ

private: // 立体描画する際に必要なもの
	ComPtr<ID3D11Buffer> m_pIndexBuffer; // インデックスデータ用のバッファ
	UINT m_indexCount = 0; // インデックスの数

private:
	D3D11_PRIMITIVE_TOPOLOGY m_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

};
