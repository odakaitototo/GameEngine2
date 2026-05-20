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
	bool Create(ID3D11Device* device, const std::vector<Vertex>& vertices);

	// Contextに、このメッシュを描画する準備をさせる
	void Bind(ID3D11DeviceContext* context);

	// 実際に描画命令を出す
	void Draw(ID3D11DeviceContext* context);

private:
	ComPtr<ID3D11Buffer> m_pvertexBuffer; // GPU側のメモリに置かれた頂点データ
	UINT m_vertexCount = 0; // 頂点の数
	UINT m_stride = sizeof(Vertex); // 頂点1つ当たりのサイズ
};
