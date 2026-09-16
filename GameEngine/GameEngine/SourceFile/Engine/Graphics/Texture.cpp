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
	desc.MipLevels = 0;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 一般的なRGBAフォーマット
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET; // シェーダーに素材として渡す設定
	desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	
	HRESULT hr = device->CreateTexture2D(&desc, nullptr, m_pTexture.GetAddressOf());


	if (FAILED(hr))
	{
		stbi_image_free(pixels); // 失敗時は解放して終了
		OutputDebugStringA("ERROR: CreateTexture2D で失敗しました！");
		return false;
	}
	// Device から Context を取得して、一番大きい画像を流し込む
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	device->GetImmediateContext(&context);
	context->UpdateSubresource(m_pTexture.Get(), 0, nullptr, pixels, width * 4, 0);


	// 不要になったCPUの画像メモリを解放
	stbi_image_free(pixels);


	// ShaderResourceViewの作成（第2引数を nullptr にすると自動で全Mipレベルが設定されます）
	hr = device->CreateShaderResourceView(m_pTexture.Get(), nullptr, m_pSRV.GetAddressOf());

	if (FAILED(hr))
	{
		OutputDebugStringA("=== ERROR: CreateShaderResourceView で失敗しました！ ===\n");
		return false;
	}
	
	context->GenerateMips(m_pSRV.Get()); // GPUに縮小版の画像を一気に生成

	// 正常に読み込めたら、パスを記憶しておく
	m_filePath = filename;

	return true;
}
