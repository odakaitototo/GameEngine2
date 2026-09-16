#pragma once
#include <string>
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <assimp/scene.h>

// 1つのキーフレーム(位置)
struct KeyPosition
{
	DirectX::XMFLOAT3 position;
	float timeStamp;
};

// 1つのキーフレーム(回転)
struct KeyRotation
{
	DirectX::XMFLOAT4 orientation; // (x,y,z,w)
	float timeStamp;
};

// 1つのキーフレーム(スケール)
struct KeyScale
{
	DirectX::XMFLOAT3 scale;
	float timeStamp;
};


// ボーンごとのアニメーションチャンネルデータ
struct BoneAnimationChannel
{
	std::string boneName;
	std::vector<KeyPosition> positions;
	std::vector<KeyRotation> rotation;
	std::vector<KeyScale> scale;
};

// アニメーションクリップ
class Animation
{
public:
	bool Load(const aiAnimation* aiAnim);

	float GetDuration() const
	{
		return m_duration;
	}

	float GetTicksPerSecond() const
	{
		return m_ticksPerSecond;
	}

	const std::vector<BoneAnimationChannel>& GetChannels() const
	{
		return m_channels;
	}

private:
	float m_duration = 0.0f; // アニメーション全体の長さ
	float m_ticksPerSecond = 25.0f; // 1秒あたりのティック数
	std::vector<BoneAnimationChannel> m_channels; // 骨ごとのアニメーションデータリスト
};