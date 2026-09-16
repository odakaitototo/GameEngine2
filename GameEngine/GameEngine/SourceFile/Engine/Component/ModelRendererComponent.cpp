#include "Engine/Component/ModelRendererComponent.h"

void ModelRendererComponent::Draw(ID3D11DeviceContext* context)
{
	// モデルがセットされていれば、パーツを全て描画
	if (model != nullptr)
	{
		model->Draw(context);
	}
}