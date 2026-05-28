#include "Engine/Graphics/Mesh.h"

bool Mesh::Create(ID3D11Device* device, const std::vector<Vertex>& vertices, const std::vector<UINT>& indices)
{
	m_vertexCount = static_cast<UINT>(vertices.size());
	m_indexCount = static_cast<UINT>(indices.size()); // インデックスの数を保存

	// 頂点バッファの設定
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
	if (FAILED(hr))
	{
		return false;
	}

	
	

	// インデックスバッファの設定
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = sizeof(UINT) * m_indexCount; // インデックスの数　* UINTのサイズ
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER; // 子俺はインデックスバッファという宣言
	ibd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA isd = {};
	isd.pSysMem = indices.data();

	hr = device->CreateBuffer(&ibd, &isd, &m_pIndexBuffer);

	return SUCCEEDED(hr);
}

void Mesh::Bind(ID3D11DeviceContext* context)
{
	// コンテキストに「今からこのバッファを使って描画するぞ」と命令する
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, m_pvertexBuffer.GetAddressOf(), &m_stride, &offset);

	// インデックスバッファをセットする（32ビットの整数フォーマット）
	context->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// 頂点をどうつないで形にするか
	context->IASetPrimitiveTopology(m_topology); // 三角形リスト
}

void Mesh::Draw(ID3D11DeviceContext* context)
{
	// 「描画しろ」という命令（ドローコール）をGPUに送る
	context->DrawIndexed(m_indexCount, 0, 0);
}