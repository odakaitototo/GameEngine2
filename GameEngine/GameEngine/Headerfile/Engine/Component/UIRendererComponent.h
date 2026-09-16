#pragma once
#include  "Engine/Component/ComponentBase.h"
#include "Engine/Graphics/Texture.h"

#include <memory>
#include <DirectXMath.h>

class UIRendererComponent : public ComponentBase
{
public:
	std::shared_ptr<Texture> texture; // 表示したい画像

	// UI用のパラメータ
	DirectX::XMFLOAT2 position = { 0.0f,0.0f }; // 画面上の表示位置(x,y)左上が(0,0)
	DirectX::XMFLOAT2 size = { 200.0f,200.0f }; // 画像の幅と高さ(ピクセル)
	DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f }; // 色と透明度
};
