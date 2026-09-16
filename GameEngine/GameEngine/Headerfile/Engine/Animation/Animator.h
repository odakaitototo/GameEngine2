#pragma once
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include "Engine/Animation/Animation.h"
#include "Engine/Graphics/Model.h"

class Animator
{
public:
    Animator();

    // 再生するアニメーションと、対象のモデルをセットする
    void PlayAnimation(std::shared_ptr<Animation> animation, const Model* model);

    // 毎フレーム呼び出して、時間（dt = DeltaTime）を進め、骨を動かす
    void UpdateAnimation(float dt);

    // 計算が完了したボーン変形行列のリスト（これをシェーダーに送ります！）
    const std::vector<DirectX::XMFLOAT4X4>& GetFinalBoneMatrices() const { return m_finalBoneMatrices; }

public: // アニメーションタイプ
    void Play()
    {
        m_isPlaying = true;
    }

    void Pause()
    {
        m_isPlaying = false;
    }

    bool IsPlaying() const
    {
        return m_isPlaying;
    }

private:
    // ツリー構造を上から下へと辿って、親の動きを子へ伝播させる再帰関数
    void CalculateBoneTransform(const AssimpNodeData* node, DirectX::XMFLOAT4X4 parentTransform);

    std::vector<DirectX::XMFLOAT4X4> m_finalBoneMatrices; // GPUに送る最終的な行列（最大100個程度）

    std::shared_ptr<Animation> m_currentAnimation;
    const Model* m_targetModel = nullptr; // 骨の階層とオフセット行列をもらうためのポインタ

    float m_currentTime;
    float m_deltaTime;

    bool m_isPlaying = true; // 再生中ラグ
    float m_playSpeed = 1.0f; // 再生速度倍率
};
