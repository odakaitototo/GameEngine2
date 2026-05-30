#include "Engine/Graphics/Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "Engine/Graphics/stb_image.h" //　画像をピクセルデータに変更してDirectXで描画できるようにするためのもの






bool Texture::Load(ID3D11Device* device, const std::string& filename)
{
	// 画像ファイルを読み込む（幅、高さ、色数を取得）
	int width, height, channels;
	//3D用のテクスチャは4チャンネルが基本（RGBA）
	unsigned char* pixels = stbi_load(filename.c_str(), &width, &height, &channels, 4);

	if (!pixels)
	{
		return false; // 画像が読み込めなかった場合失敗
	}

	// DirectX用のテクスチャの設定を作る
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 一般的なRGBAフォーマット
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // シェーダーに素材として渡す設定

	// 読み込んだ画像データをDirectXで描画する準備
	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pixels;
	initData.SysMemPitch = width * 4; // 1行当たりのデータサイズ

	// GPU上にテクスチャの実態を作成
	HRESULT hr = device->CreateTexture2D(&desc, &initData, &m_pTexture);

	// GPUに画像のメモリ情報を渡したので
	// 不要になったCPU側のメモリ情報を削除する
	stbi_image_free(pixels);

	if (FAILED(hr))
	{
		return false;
	}

	// テクスチャをシェーダー（HLSL）で使うためのビュー（SRV）を作る
	hr = device->CreateShaderResourceView(m_pTexture.Get(), nullptr, &m_pSRV);

	if (FAILED(hr))
	{
		return false;
	}

	// 正常に読み込めたら、パスを記憶しておく」
	m_filePath = filename;

	return true;
}
