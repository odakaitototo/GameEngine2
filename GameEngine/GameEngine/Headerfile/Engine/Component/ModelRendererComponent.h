#pragma once
#include "Engine/Component/ComponentBase.h"
#include "Engine/Graphics/Model.h"
#include "Engine/Graphics/Texture.h"
#include "Engine/System/Time/Time.h"
#include "Engine/Animation/Animator.h"
#include <memory>
#include <directXMath.h>


class ModelRendererComponent : public ComponentBase
{
public:
	ModelRendererComponent() = default;
	~ModelRendererComponent() = default;

	// ComponentBaseの必要関数
	// 毎フレーム自動でアニメーションを再生させる
	void Update() override
	{
		if (model && model->GetAnimation())
		{
			
			// 時間を進める
			animator->UpdateAnimation(Time::GetDeltaTime());
		}
	}


	void LateUpdate() override{}

	// 描画処理
	void Draw(ID3D11DeviceContext* context);


	// FBX等のモデルデータを保存
	std::shared_ptr<Model> model = nullptr;

	// モデル全体に適用するテクスチャ
	std::shared_ptr<Texture> texture = nullptr;

	// 色情報
	DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f };
	bool useSolidColor = false;

	std::shared_ptr<Animator> animator = std::make_shared<Animator>();

	
	

};