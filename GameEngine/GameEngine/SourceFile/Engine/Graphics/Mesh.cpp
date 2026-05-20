#include "Engine/Graphics/Mesh.h"

bool Mesh::Create(ID3D11Device* device, const std::vector<Vertex>& vertices)
{
	m_vertexCount = static_cast<UINT>(vertices.size());

	// バッファの設定
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT; // GPUが読み書きする標準的な設定
	bd.ByteWidth = sizeof(Vertex) * m_vertexCount; // バッファの合計サイズ（バイト）
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; // 「これは頂点バッファ」という宣言
	bd.CPUAccessFlags = 0; // CPUからは後で書き換えない（高速化）

	// 初期データの設定
	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices.data(); // メモリ上の頂点データの先頭住所

	// GPU上にバッファを（メモリ領域）確保
	// Deviceに依頼してGPUのメモリに頂点データを転送し、m_pvertexBufferに保存します。
	HRESULT hr = device->CreateBuffer(&bd, &sd, &m_pvertexBuffer);
	return SUCCEEDED(hr);
}

void Mesh::Bind(ID3D11DeviceContext* context)
{
	// コンテキストに「今からこのバッファを使って描画するぞ」と命令する
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, m_pvertexBuffer.GetAddressOf(), &m_stride, &offset);

	// 頂点をどうつないで形にするか
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 三角形リスト
}

void Mesh::Draw(ID3D11DeviceContext* context)
{
	// 「描画しろ」という命令（ドローコール）をGPUに送る
	context->Draw(m_vertexCount, 0);
}