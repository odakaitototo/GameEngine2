#include "Engine/Core/Renderer.h"
#include "Engine/Core/Application.h"
#include <windows.h>


void Renderer::Render(Application* app)
{
    // ここで今後のUpdateやDrawを呼び出します

    app->m_shader->Bind(app->m_dx.GetContext()); // シェイダーを使うためにGPUに指示する

    // Cameraクラスから完成済みの行列を持ってくる
    DirectX::XMMATRIX viewMatrix = app->m_camera.GetViewMatrix();
    DirectX::XMMATRIX projectionMatrix = app->m_camera.GetProjectionMatrix();


    // シーンに存在する全てのゲームオブジェクトをループ描画する
    for (int i = 0; i < app->m_gameObjects.size(); i++)
    {
        // Transformを取得
        auto& t = app->m_gameObjects[i]->GetTransform();

        // スケール・回転・平行移動の行数を作成
        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z);

        float radX = DirectX::XMConvertToRadians(t.rotation.x);
        float radY = DirectX::XMConvertToRadians(t.rotation.y);
        float radZ = DirectX::XMConvertToRadians(t.rotation.z);
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(radX, radY, radZ);
        DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(t.position.x, t.position.y, t.position.z);

        // 3つの行数を掛け合わせてワールド行列を完成させる
        DirectX::XMMATRIX worldMatrix = scale * rotation * translation;

        // 定数バッファの構造体にデータを詰める
        ConstantBufferTransform cbData;

        // HLSLは行列の読み込み方法がC++と逆なので、転置して送る
        cbData.worldMatrix = DirectX::XMMatrixTranspose(worldMatrix);

        // ViewとProjectionも転置して詰める
        cbData.viewMatrix = DirectX::XMMatrixTranspose(viewMatrix);
        cbData.projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);

        D3D11_MAPPED_SUBRESOURCE mappedResource;

        HRESULT hr = app->m_dx.GetContext()->Map(app->m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

        // D3D11_MAP_WRITE_DISCARDが重要
        // 「前の箱はGPUが使っているかもしれないから、古いのは破棄して、新しい箱を用意して」という命令
        if (SUCCEEDED(hr))
        {
            // もらった新しい箱(pData)に、行列データを直接流し込む
            memcpy(mappedResource.pData, &cbData, sizeof(ConstantBufferTransform));

            // 箱を閉じる
            app->m_dx.GetContext()->Unmap(app->m_pConstantBuffer.Get(), 0);

        }



        // 「0番目のスロット(b0)」ここの定数バッファをセットする
        app->m_dx.GetContext()->VSSetConstantBuffers(0, 1, app->m_pConstantBuffer.GetAddressOf());

        // データの準備完了　描画
        app->m_gameObjects[i]->Draw(app->m_dx.GetContext());


    }
}
