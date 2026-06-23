#pragma once
#include "Engine/Component/ComponentBase.h"
#include "Engine/Graphics/Mesh.h"
#include "Engine/Graphics/Texture.h"
#include "DirectXMath.h"
#include <memory>

class MeshRendererComponent : public ComponentBase
{
public:
	MeshRendererComponent() = default;
	~MeshRendererComponent() = default;

	// Œ©‚½–Ú‚ÉŠÖ‚·‚éƒf[ƒ^
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Texture> texture;
	DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f };
	bool useSolidColor = false;

	// •`‰æˆ—
	void Draw(ID3D11DeviceContext* context)
	{
		if (mesh)
		{
			mesh->Bind(context);
			mesh->Draw(context);
		}
	}

};