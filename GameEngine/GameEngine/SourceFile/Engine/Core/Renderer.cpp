#include "Engine/Core/Renderer.h"
#include "Engine/Core/Application.h"
#include <windows.h>


void Renderer::Render(Application* app)
{
    // ここで今後のUpdateやDrawを呼び出します

    app->m_shader->Bind(app->m_dx.GetContext()); // シェイダーを使うためにGPUに指示する

    // Cameraクラスから完成済みの行列を持ってくる
    DirectX::XMMATRIX viewMatrix = app->GetCurrentViewMatrix();
    DirectX::XMMATRIX projectionMatrix = app->GetCurrentProjectionMatrix();

    // グリッド線を表示
    if (app->m_gridMesh)
    {
        ConstantBufferTransform cbGrid;

        // グリッドは空間の中心から動かさないので、単位行列のまま送る
        cbGrid.worldMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());

        cbGrid.viewMatrix = DirectX::XMMatrixTranspose(viewMatrix);
        cbGrid.projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);

        // 色の設定
        cbGrid.materialColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        cbGrid.useSolidColor = 2;
        cbGrid.dummy = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

        // 定数バッファにデータを書き込む
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hrGrid = app->m_dx.GetContext()->Map(app->m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

        if (SUCCEEDED(hrGrid))
        {
            memcpy(mappedResource.pData, &cbGrid, sizeof(ConstantBufferTransform));
            app->m_dx.GetContext()->Unmap(app->m_pConstantBuffer.Get(), 0);
        }

        // シェーダーにデータを送る
        app->m_dx.GetContext()->VSSetConstantBuffers(0, 1, app->m_pConstantBuffer.GetAddressOf());
        app->m_dx.GetContext()->PSSetConstantBuffers(0,1,app->m_pConstantBuffer.GetAddressOf());

        // 描画処理
        app->m_gridMesh->Bind(app->m_dx.GetContext());
        app->m_gridMesh->Draw(app->m_dx.GetContext());

        
    }

    


    // シーンに存在する全てのゲームオブジェクトをループ描画する
    for (int i = 0; i < app->m_gameObjects.size(); i++)
    {
        // Transformを取得
        auto& t = app->m_gameObjects[i]->GetTransform();

        // すでにUpdateで計算済みの「ワールド行列」をそのまま読み込む！
        DirectX::XMMATRIX worldMatrix = DirectX::XMLoadFloat4x4(&t.worldMatrix);

        // 定数バッファの構造体にデータを詰める
        ConstantBufferTransform cbData;

        cbData.worldMatrix = DirectX::XMMatrixTranspose(worldMatrix);


        // ViewとProjectionも転置して詰める
        cbData.viewMatrix = DirectX::XMMatrixTranspose(viewMatrix);
        cbData.projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);

        // GameObjectの色を、GPUへ送るデータに詰める
        cbData.materialColor = app->m_gameObjects[i]->GetColor();

        if (app->m_gameObjects[i]->GetUseSolidColor())
        {
            cbData.useSolidColor = 1; // 単色モード
        }
        else if(app->m_gameObjects[i]->GetTexture())
        {
            cbData.useSolidColor = 0; // テクスチャーモード
        }
        else
        {
            cbData.useSolidColor = 2; // 画像がない場合は頂点カラーモード
        }
        //  boolをintに変換してGPUに送る
       // cbData.useSolidColor = app->m_gameObjects[i]->GetUseSolidColor() ? 1 : 0;

        cbData.dummy = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
            
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

        // ピクセルシェイダーにも上と同じデータをセットする
        app->m_dx.GetContext()->PSSetConstantBuffers(0, 1, app->m_pConstantBuffer.GetAddressOf());

        // GameObjectが画像を持っていたら、シェーダーにセットする
        auto texture = app->m_gameObjects[i]->GetTexture();
        if (texture != nullptr)
        {
            // 画像を持っている場合は、その画像をセット
            ID3D11ShaderResourceView* srv = texture->GetSRV();
            app->m_dx.GetContext()->PSSetShaderResources(0, 1, &srv);
        }
        else
        {
            ID3D11ShaderResourceView* nullSrv = nullptr;
            app->m_dx.GetContext()->PSSetShaderResources(0, 1, &nullSrv);
        }

        // データの準備完了　描画
        app->m_gameObjects[i]->Draw(app->m_dx.GetContext());

    }

    if (app->GetEngineMode() == EngineMode::Editor)
    {
        m_debugRenderer.DrawColliders(app, viewMatrix, projectionMatrix);
    }
}
