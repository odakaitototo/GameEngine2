#include "Engine/Animation/Animation.h"

bool Animation::Load(const aiAnimation* aiAnim)
{
	if (!aiAnim)
	{
		return false;
	}

	// アニメーション名や全体の長さを取得
	m_duration = static_cast<float>(aiAnim->mDuration);
	m_ticksPerSecond = static_cast<float>(aiAnim->mTicksPerSecond);

	// 1秒あたりのティック数が0の場合はデフォルト値を設定
	if (m_ticksPerSecond == 0.0f)
	{
		m_ticksPerSecond = 25.0f; // デフォルト値として25.0秒を設定
	}

	// 各ボーンごとのアニメーションデータを抽出
	for (unsigned int i = 0; i < aiAnim->mNumChannels; i++)
	{
		aiNodeAnim* aiNodeAnim = aiAnim->mChannels[i];
		BoneAnimationChannel channel;
		channel.boneName = aiNodeAnim->mNodeName.data;

		// 位置のキーフレームを抽出
		for (unsigned int p = 0; p < aiNodeAnim->mNumPositionKeys; p++)
		{
			aiVector3D aiPos = aiNodeAnim->mPositionKeys[p].mValue;
			float timeStamp = static_cast<float>(aiNodeAnim->mPositionKeys[p].mTime);

			KeyPosition keyPos;
			keyPos.position = DirectX::XMFLOAT3(aiPos.x, aiPos.y, aiPos.z);
			keyPos.timeStamp = timeStamp;
			channel.positions.push_back(keyPos);
		}

		// 回転のキーフレーム抽出
		for (unsigned int r = 0; r < aiNodeAnim->mNumRotationKeys; r++)
		{
			aiQuaterniont aiRot = aiNodeAnim->mRotationKeys[r].mValue;
			float timeStamp = static_cast<float>(aiNodeAnim->mRotationKeys[r].mTime);

			KeyRotation keyRot;
			// Assimpのクォータニオン(x,y,z,w)をDirectX側に合わせる
			keyRot.orientation = DirectX::XMFLOAT4(aiRot.x, aiRot.y, aiRot.z, aiRot.w);
			keyRot.timeStamp = timeStamp;
			channel.rotation.push_back(keyRot);
		}

		// スケールのキーフレームを抽出
		for (unsigned int s = 0; s < aiNodeAnim->mNumScalingKeys; s++)
		{
			aiVector3D aiScale = aiNodeAnim->mScalingKeys[s].mValue;
			float timeStamp = static_cast<float>(aiNodeAnim->mScalingKeys[s].mTime);

			KeyScale keyScale;
			keyScale.scale = DirectX::XMFLOAT3(aiScale.x, aiScale.y, aiScale.z);
			keyScale.timeStamp = timeStamp;
			channel.scale.push_back(keyScale);
		}

		// 完成したチャンネルをリストに追加
		m_channels.push_back(channel);
	}
	return true;
}
	