#include "Engine/Graphics/Renderer.h"
#include "Engine/Core/Application.h"
#include "Engine/Component/ModelRendererComponent.h"
#include "Engine/Component/UIRendererComponent.h"
#include <windows.h>


void Renderer::Render(Application* app)
{
    // ここで今後のUpdateやDrawを呼び出します

    app->m_shader->Bind(app->m_dx.GetContext()); // シェイダーを使うためにGPUに指示する

    // ボーン（アニメーション）用定数バッファの送信
    BoneBuffer cbBone;

    // 全ての骨を「変化なし（単位行列）」で初期化
    // （これをしないと、アニメーションを持たない普通のオブジェクトまで潰れて消えてしまいます）
    DirectX::XMFLOAT4X4 identity = { 1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1 };
    for (int b = 0; b < 100; b++)
    {
        cbBone.transforms[b] = identity;
    }

    // GPU(スロット1番 b1)へデータを転送してセット
    app->m_dx.GetContext()->UpdateSubresource(app->m_pBoneBuffer.Get(), 0, nullptr, &cbBone, 0, 0);
    app->m_dx.GetContext()->VSSetConstantBuffers(1, 1, app->m_pBoneBuffer.GetAddressOf());

    // Cameraクラスから完成済みの行列を持ってくる
    DirectX::XMMATRIX viewMatrix = app->GetCurrentViewMatrix();
    DirectX::XMMATRIX projectionMatrix = app->GetCurrentProjectionMatrix();

    // グリッド線を表示
    if (app->m_gridMesh && app->GetEngineMode() == EngineMode::Editor)
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
        //UIオブジェクトを3D]」オブジェクトを描画後に描画するのでここではッスキップする
        if (app->m_gameObjects[i]->GetComponent<UIRendererComponent>())
        {
            continue;
        }

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

        BoneBuffer cbBone;
        DirectX::XMFLOAT4X4 identity = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
        for (int b = 0; b < 100; b++) { cbBone.transforms[b] = identity; }

        auto modelRenderer = app->m_gameObjects[i]->GetComponent<ModelRendererComponent>();
        if (modelRenderer && modelRenderer->model && modelRenderer->model->GetAnimation())
        {
            auto& realBoneMatrices = modelRenderer->animator->GetFinalBoneMatrices();
            for (int b = 0; b < 100; b++)
            {
                cbBone.transforms[b] = realBoneMatrices[b];
            }
        }

        app->m_dx.GetContext()->UpdateSubresource(app->m_pBoneBuffer.Get(), 0, nullptr, &cbBone, 0, 0);
        app->m_dx.GetContext()->VSSetConstantBuffers(1, 1, app->m_pBoneBuffer.GetAddressOf());

        // データの準備完了　描画
        app->m_gameObjects[i]->Draw(app->m_dx.GetContext());

    }

    if (app->GetEngineMode() == EngineMode::Editor)
    {
        m_debugRenderer.DrawColliders(app, viewMatrix, projectionMatrix);
    }
        // モード切替(Zテストを無視して最前面に、α透過を有効に)
        app->m_dx.SetUIMode();

        // 直前に描画した3Dキャラクターの骨の姿勢が歪まないように、リセット
        BoneBuffer cbBoneIdentity;
        DirectX::XMFLOAT4X4 matIdentity = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
        for (int b = 0; b < 100; b++) cbBoneIdentity.transforms[b] = matIdentity;
        app->m_dx.GetContext()->UpdateSubresource(app->m_pBoneBuffer.Get(), 0, nullptr, &cbBoneIdentity, 0, 0);

        // 2Dカメラの行列
        // 左上がx:0, y:0になる右下が画面幅・画面高さになるピクセル基準の空間を作る
        DirectX::XMMATRIX uiViewMatrix = DirectX::XMMatrixIdentity();
        DirectX::XMMATRIX uiProjMatrix = DirectX::XMMatrixOrthographicOffCenterLH(0.0f, app->m_screenWidth, app->m_screenHeight, 0.0f, 0.0f, 1.0f);

        // UIコンポーネントを持つオブジェクトだけを探して、板ポリゴンを書く
        for (int i = 0; i < app->m_gameObjects.size(); i++)
        {
            auto ui = app->m_gameObjects[i]->GetComponent<UIRendererComponent>();
            if (!ui || !ui->texture)
            {
                continue; //UIコンポーネントがないかまたは画像がない場合スキップ
            }

            ConstantBufferTransform cbUI;

            // 画面の幅と高さを取得（キャンバスサイズ）
            float screenW = app->m_sceneWidth;
            float screenH = app->m_sceneHeight;
            // 位置の計算 パーセント(0.0～1.0) を実際のピクセルに変換
            float posX = screenW * ui->position.x;
            float posY = screenH * ui->position.y;
            float baseScale = screenH / 100.0f;
            float sizeW = ui->size.x * baseScale;
            float sizeH = ui->size.y * baseScale;
          
            DirectX::XMMATRIX centerOffset = DirectX::XMMatrixTranslation(-0.5f, -0.5f, 0.0f);
            DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(sizeW, sizeH, 1.0f);
            DirectX::XMMATRIX translate = DirectX::XMMatrixTranslation(posX, posY, 0.0f);
            cbUI.worldMatrix = DirectX::XMMatrixTranspose(centerOffset * scale * translate);

            // 投影行列の計算
            DirectX::XMMATRIX uiViewMatrix = DirectX::XMMatrixIdentity();
            DirectX::XMMATRIX uiProjMatrix = DirectX::XMMatrixOrthographicOffCenterLH(0.0f, app->m_sceneWidth, app->m_sceneHeight, 0.0f, 0.0f, 1.0f);

            cbUI.viewMatrix = DirectX::XMMatrixTranspose(uiViewMatrix);
            cbUI.projectionMatrix = DirectX::XMMatrixTranspose(uiProjMatrix);

            cbUI.materialColor = ui->color;
            cbUI.useSolidColor = 0; // テクスチャを使用
            cbUI.dummy = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
            // GPUへUIの行列データを送る
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            if (SUCCEEDED(app->m_dx.GetContext()->Map(app->m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
            {
                memcpy(mappedResource.pData, &cbUI, sizeof(ConstantBufferTransform));
                app->m_dx.GetContext()->Unmap(app->m_pConstantBuffer.Get(), 0);
            }

            // 定数バッファと画像をシェーダーにセット
            app->m_dx.GetContext()->VSSetConstantBuffers(0, 1, app->m_pConstantBuffer.GetAddressOf());
            app->m_dx.GetContext()->PSSetConstantBuffers(0, 1, app->m_pConstantBuffer.GetAddressOf());
            ID3D11ShaderResourceView* srv = ui->texture->GetSRV();
            app->m_dx.GetContext()->PSSetShaderResources(0, 1, &srv);
            // 作っておいた1x1サイズの「UI用の板ポリゴン」を使って描画！
            app->m_uiQuadMesh->Bind(app->m_dx.GetContext());
            app->m_uiQuadMesh->Draw(app->m_dx.GetContext());
        }

        // 最後に通常の3Dモードに戻して、次のフレームの描画に備える
        app->m_dx.Set3DMode();
        
    
}