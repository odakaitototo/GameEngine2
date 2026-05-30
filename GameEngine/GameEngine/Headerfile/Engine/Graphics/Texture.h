#pragma once
#include <d3d11.h>
#include<wrl/client.h>
#include<string>

// 画像のデータ読み込み、GPU用のリソース（SRV）として管理するクラス
class Texture
{
public:
	Texture() = default;
	~Texture() = default;

	// 画像ファイル（PNGやJGなど）を読み込む
	bool Load(ID3D11Device* device, const std::string& filename);

	// 描画する時に、GPU（シェーダー）に画像を渡すための窓口
	ID3D11ShaderResourceView* GetSRV() const { return m_pSRV.Get(); }

private:

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pTexture; // テクスチャの実態
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pSRV; // シェーダーへ渡す用のビュー

};
