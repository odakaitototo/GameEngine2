#include "Engine/Animation/Animator.h"
#include <cmath>


// アニメーションキーフレームを検索する関数
namespace
{
    int GetPositionIndex(const BoneAnimationChannel& channel, float animationTime)
    {
        for (size_t i = 0; i < channel.positions.size() - 1; ++i) 
        {
            if (animationTime < channel.positions[i + 1].timeStamp) return static_cast<int>(i);
        }
        return 0;
    }
    int GetRotationIndex(const BoneAnimationChannel& channel, float animationTime)
    {
        for (size_t i = 0; i < channel.rotation.size() - 1; ++i)
        {
            if (animationTime < channel.rotation[i + 1].timeStamp) return static_cast<int>(i);
        }
        return 0;
    }
    int GetScaleIndex(const BoneAnimationChannel& channel, float animationTime) 
    {
        for (size_t i = 0; i < channel.scale.size() - 1; ++i)
        {
            if (animationTime < channel.scale[i + 1].timeStamp) return static_cast<int>(i);
        }
        return 0;
    }
}


// Animator クラスの実装
Animator::Animator()
{
    m_currentTime = 0.0f;
    m_deltaTime = 0.0f;

    // ボーン変形行列を100個分（最大100ボーンまで対応）の単位行列で初期化
    DirectX::XMFLOAT4X4 identity = 
    {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    m_finalBoneMatrices.resize(100, identity);
}

void Animator::PlayAnimation(std::shared_ptr<Animation> animation, const Model* model)
{
    m_currentAnimation = animation;
    m_targetModel = model;
    m_currentTime = 0.0f; // 再生時間をリセット
}

void Animator::UpdateAnimation(float dt)
{
    m_deltaTime = dt;
    if (m_currentAnimation && m_targetModel)
    {
        // 時間を進める
        m_currentTime += dt * m_currentAnimation->GetTicksPerSecond();
        m_currentTime = fmod(m_currentTime, m_currentAnimation->GetDuration());

        // 基準となる一番大元の行列（単位行列）
        DirectX::XMFLOAT4X4 identity = {
            1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1
        };

        // 根元の骨から計算スタート
        CalculateBoneTransform(&m_targetModel->GetRootNode(), identity);
    }
}

void Animator::CalculateBoneTransform(const AssimpNodeData* node, DirectX::XMFLOAT4X4 parentTransform)
{
    std::string nodeName = node->name;
    DirectX::XMMATRIX nodeTransformMat = DirectX::XMLoadFloat4x4(&node->transformation); // 初期行列をロード

    
    // アニメーションの計算を行って上書きする
    const BoneAnimationChannel* channel = nullptr;
    for (const auto& c : m_currentAnimation->GetChannels())
    {
        if (c.boneName == nodeName) 
        { 
            channel = &c; break;
        }
    }

    if (channel)
    {
        // 1. 位置 (Lerp)
        DirectX::XMVECTOR finalPos;
        if (channel->positions.size() == 1)
        { 
            finalPos = DirectX::XMLoadFloat3(&channel->positions[0].position); 
        }
        else 
        {
            int p0 = GetPositionIndex(*channel, m_currentTime);
            float factor = (m_currentTime - channel->positions[p0].timeStamp) / (channel->positions[p0 + 1].timeStamp - channel->positions[p0].timeStamp);
            finalPos = DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&channel->positions[p0].position), DirectX::XMLoadFloat3(&channel->positions[p0 + 1].position), factor);
        }

        // 回転 (Slerp)
        DirectX::XMVECTOR finalRot;
        if (channel->rotation.size() == 1) 
        { 
            finalRot = DirectX::XMLoadFloat4(&channel->rotation[0].orientation);
        }
        else
        {
            int r0 = GetRotationIndex(*channel, m_currentTime);
            float factor = (m_currentTime - channel->rotation[r0].timeStamp) / (channel->rotation[r0 + 1].timeStamp - channel->rotation[r0].timeStamp);
            finalRot = DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&channel->rotation[r0].orientation), DirectX::XMLoadFloat4(&channel->rotation[r0 + 1].orientation), factor);
        }

        // スケール (Lerp)
        DirectX::XMVECTOR finalScale;
        if (channel->scale.size() == 1) 
        { 
            finalScale = DirectX::XMLoadFloat3(&channel->scale[0].scale);
        }
        else 
        {
            int s0 = GetScaleIndex(*channel, m_currentTime);
            float factor = (m_currentTime - channel->scale[s0].timeStamp) / (channel->scale[s0 + 1].timeStamp - channel->scale[s0].timeStamp);
            finalScale = DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&channel->scale[s0].scale), DirectX::XMLoadFloat3(&channel->scale[s0 + 1].scale), factor);
        }

        // スケール × 回転 × 位置 の順番で合成して、nodeTransformMatをアニメーション用の行列に上書き
        nodeTransformMat = DirectX::XMMatrixScalingFromVector(finalScale) * DirectX::XMMatrixRotationQuaternion(finalRot) * DirectX::XMMatrixTranslationFromVector(finalPos);
    }

    // 自分の行列 × 親の行列 ＝ グローバル行列
    DirectX::XMMATRIX globalTransform = nodeTransformMat * DirectX::XMLoadFloat4x4(&parentTransform);

    Model* model = const_cast<Model*>(m_targetModel);
    auto& boneInfoMap = model->GetBoneInfoMap();

    // 最終変形行列を保存（GPUに送る用）
    if (boneInfoMap.find(nodeName) != boneInfoMap.end())
    {
        int index = boneInfoMap[nodeName].id;
        DirectX::XMMATRIX offset = DirectX::XMLoadFloat4x4(&boneInfoMap[nodeName].offsetMatrix);
        DirectX::XMMATRIX finalMatrix = offset * globalTransform;

        // GPUに送るために転置(Transpose)する
        DirectX::XMStoreFloat4x4(&m_finalBoneMatrices[index], DirectX::XMMatrixTranspose(finalMatrix));
    }

    // 子供へ伝播
    DirectX::XMFLOAT4X4 nextParentTransform;
    DirectX::XMStoreFloat4x4(&nextParentTransform, globalTransform);

    for (int i = 0; i < node->children.size(); i++)
    {
        CalculateBoneTransform(&node->children[i], nextParentTransform);
    }
}