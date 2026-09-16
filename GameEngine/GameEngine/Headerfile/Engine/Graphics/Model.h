#pragma once
#include "Engine/Graphics/Mesh.h"
#include "Engine/Animation/Animation.h"
#include <string>
#include <vector>
#include <memory>
#include <d3d11.h>
#include <map>
#include <DirectXMath.h>

struct aiNode;

// ボーンごとのIDとオフセット行列を保持する構造体
struct BoneInfo
{
	int id;
	DirectX::XMFLOAT4X4 offsetMatrix;
};

// 骨の親子関係を記憶するツリー構造
struct AssimpNodeData
{
	std::string name;
	DirectX::XMFLOAT4X4 transformation; // 初期状態のローカル行列
	std::vector<AssimpNodeData> children; // 子
};

class Model
{
public:
	// FBXを読み込んで、内部で複数のMeshを作成する
	bool Load(ID3D11Device* device, const std::string& filename);

	// 持っている全てのパーツ(Mesh)を一括で描画する
	void Draw(ID3D11DeviceContext* context);

	const std::string& GetFilePath() const { return m_filePath; }

	std::shared_ptr<Animation> GetAnimation() const
	{
		return m_animation;
	}

	// Animatorから骨の情報を受け取るためのゲッター
	std::map<std::string, BoneInfo>& GetBoneInfoMap()
	{
		return m_boneInfoMap;
	}

	const AssimpNodeData& GetRootNode() const
	{
		return m_rootNode;
	}

	

private:
	// 分割されたパーツをすべて保存するリスト
	std::vector<std::shared_ptr<Mesh>> m_meshes;
	std::string m_filePath; // 読み込んだファイルのパスを保存する変数

	std::shared_ptr<Animation> m_animation = nullptr; // アニメーションデータを保存する変数

	// 骨の構造データを保存する変数
	std::map<std::string, BoneInfo> m_boneInfoMap; // ボーン名とBoneInfoの辞書
	int m_boneCounter = 0; // ボーンに被りがないIDを割り当てるためのカウンター
	AssimpNodeData m_rootNode; // 骨の親子関係のトップ

	void ReadNodeHierarchy(AssimpNodeData& dest, const aiNode* src);
};
