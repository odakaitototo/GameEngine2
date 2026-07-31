#include "Engine/Core/DebugRenderer.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Renderer.h"
#include "Engine/Component/AABBColliderComponent.h"
#include "Engine/Component/OBBColliderComponent.h"

void DebugRenderer::InitializeIfNeeded(ID3D11Device* device)
{
	// 既に初期化されていたら何もしない
	if (m_isInitialized)
	{
		return;
	}

	D3D11_RASTERIZER_DESC rsDesc = {};

	// 線だけを描画する設定
	rsDesc.FillMode = D3D11_FILL_WIREFRAME;
	rsDesc.CullMode = D3D11_CULL_NONE;
	device->CreateRasterizerState(&rsDesc, &m_wireFrameRS);

	// 塗りつぶし設定
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_BACK;
	device->CreateRasterizerState(&rsDesc, &m_solidRS);

	m_isInitialized = true;

}

void DebugRenderer::DrawColliders(Application* app, DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix)
{
    // 必要なら初期化を行う（アプリ起動後、最初にここを通った時だけ実行される）
    InitializeIfNeeded(app->m_dx.GetDevice());

    auto context = app->m_dx.GetContext();

    // GPUを「線だけ描画モード」に切り替え
    context->RSSetState(m_wireFrameRS.Get());

    // 全オブジェクトの当たり判定を描画
    for (auto& obj : app->m_gameObjects)
    {
        auto collider = obj->GetComponent < ColliderBase > ();
        if (collider)
        {

            DirectX::XMMATRIX boxWorld;
            DirectX::XMFLOAT4 wireColor;

            /////////////////
            //
            // AABBの場合
            //
            ////////////////
            if (collider->GetColliderType() == ColliderType::AABB)
            {
                // AABBCOっぃでｒ形に変換してデータを取り出す
                auto aabb = std::static_pointer_cast<AABBColliderComponent>(collider);
                auto& box = aabb->worldBoundingBox;

                DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(box.Extents.x * 2.3f, box.Extents.y * 2.3f, box.Extents.z * 2.3f);
                DirectX::XMMATRIX transMat = DirectX::XMMatrixTranslation(box.Center.x, box.Center.y, box.Center.z);
                boxWorld = scaleMat * transMat;

                wireColor = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
            }
            //////////////
            // 
            // OBBの場合
            // 
            //////////////
            else if (collider->GetColliderType() == ColliderType::OBB)
            {
                // OBBCollider型に変換してデータを取り出す
                auto obb = std::static_pointer_cast<OBBColliderComponent>(collider);
                auto& box = obb->worldBoundingBox;

                DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(box.Extents.x * 2.3f, box.Extents.y * 2.3f, box.Extents.z * 2.3f);
                DirectX::XMMATRIX transMat = DirectX::XMMatrixTranslation(box.Center.x, box.Center.y, box.Center.z);
                DirectX::XMVECTOR quat = DirectX::XMLoadFloat4(&box.Orientation);
                DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationQuaternion(quat);

                boxWorld = scaleMat * rotMat * transMat;

                wireColor = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f); // 赤色

            }
            

            // 定数バッファにデータを詰める
            ConstantBufferTransform cbData;
            cbData.worldMatrix = DirectX::XMMatrixTranspose(boxWorld);
            cbData.viewMatrix = DirectX::XMMatrixTranspose(viewMatrix);
            cbData.projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);

            cbData.materialColor = wireColor; // 当たり判定のワイヤーフレームの色
            cbData.useSolidColor = 1; // 単色モード
            cbData.dummy = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

            // バッファを更新してGPUへ送る
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            if (SUCCEEDED(context->Map(app->m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
            {
                memcpy(mappedResource.pData, &cbData, sizeof(ConstantBufferTransform));
                context->Unmap(app->m_pConstantBuffer.Get(), 0);
            }

            context->VSSetConstantBuffers(0, 1, app->m_pConstantBuffer.GetAddressOf());
            context->PSSetConstantBuffers(0, 1, app->m_pConstantBuffer.GetAddressOf());

            // テクスチャは使わないので null をセット
            ID3D11ShaderResourceView* nullSrv = nullptr;
            context->PSSetShaderResources(0, 1, &nullSrv);

            // 共通のキューブメッシュを「線」として描画
            if (app->m_commonMesh)
            {
                app->m_commonMesh->Bind(context);
                app->m_commonMesh->Draw(context);
            }
        }
    }

    // 塗りつぶしモードに戻す
    context->RSSetState(m_solidRS.Get());
}