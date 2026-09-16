#include "Engine/Graphics/Model.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#include <windows.h>
#pragma comment(lib, "assimp-vc142-mtd.lib")

// Assimpの行列をDirectXの行列に変換する関数
DirectX::XMFLOAT4X4 ConvertMatrixToDirectXFormat(const aiMatrix4x4& aiMat)
{
	DirectX::XMFLOAT4X4 mat;
	mat.m[0][0] = aiMat.a1; mat.m[0][1] = aiMat.b1; mat.m[0][2] = aiMat.c1; mat.m[0][3] = aiMat.d1;
	mat.m[1][0] = aiMat.a2; mat.m[1][1] = aiMat.b2; mat.m[1][2] = aiMat.c2; mat.m[1][3] = aiMat.d2;
	mat.m[2][0] = aiMat.a3; mat.m[2][1] = aiMat.b3; mat.m[2][2] = aiMat.c3; mat.m[2][3] = aiMat.d3;
	mat.m[3][0] = aiMat.a4; mat.m[3][1] = aiMat.b4; mat.m[3][2] = aiMat.c4; mat.m[3][3] = aiMat.d4;
	return mat;
}

bool Model::Load(ID3D11Device* device, const std::string& filename)
{
	m_filePath = filename; // パスを記憶

	Assimp::Importer importer;

	// DirectX用に最適化して読み込む
	const aiScene* scene = importer.ReadFile
	(   
		filename,
		aiProcess_Triangulate | // GPUで描画するために四角形以上のポリゴンを三角形のポリゴンにする
		aiProcess_ConvertToLeftHanded | // 左手座標系に変換する
		aiProcess_JoinIdenticalVertices // 重なっている頂点を結合する
	);

	// 読み込み失敗チェック
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::string err = "FBX LoadError:" + std::string(importer.GetErrorString());
		OutputDebugStringA(err.c_str());
		return false;
	}

	// FBX内の全てのメッシュをループで取り出す
	for (unsigned int i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh* ai_mesh = scene->mMeshes[i];

		std::vector<Vertex> vertices;
		std::vector<UINT> indices;

		// 頂点データの抽出
		for (unsigned int v = 0; v < ai_mesh->mNumVertices; v++)
		{
			Vertex vertex;

			// 座標
			vertex.pos.x = ai_mesh->mVertices[v].x;
			vertex.pos.y = ai_mesh->mVertices[v].y;
			vertex.pos.z = ai_mesh->mVertices[v].z;

			// 色
			if (ai_mesh->mColors[0])
			{
				vertex.color.x = ai_mesh->mColors[0][v].r;
				vertex.color.y = ai_mesh->mColors[0][v].g;
				vertex.color.z = ai_mesh->mColors[0][v].b;
				vertex.color.w = ai_mesh->mColors[0][v].a;
			}
			else
			{
				vertex.color = { 1.0f,1.0f,1.0f,1.0f }; // デフォルト色として白を設定
			}

			// UV座標
			if (ai_mesh->mTextureCoords[0])
			{
				vertex.uv.x = ai_mesh->mTextureCoords[0][v].x;
				vertex.uv.y = ai_mesh->mTextureCoords[0][v].y;
			}
			else
			{
				vertex.uv = { 0.0f,0.0f };
			}

			vertices.push_back(vertex);
		}

		//  1つの頂点が最大4つの骨から影響を受けるようにウェイトを割り当てる
		std::vector<int> weightCounts(vertices.size(), 0); // 各頂点に何個のボーンが割り当てられたかカウント

		for (unsigned int b = 0; b < ai_mesh->mNumBones; b++)
		{
			aiBone* bone = ai_mesh->mBones[b];
			std::string boneName = bone->mName.data;
			int boneID = -1;

			// このボーンがまだマップに登録されていなければ新規登録
			if (m_boneInfoMap.find(boneName) == m_boneInfoMap.end())
			{
				BoneInfo newBoneInfo;
				newBoneInfo.id = m_boneCounter;
				newBoneInfo.offsetMatrix = ConvertMatrixToDirectXFormat(bone->mOffsetMatrix); // オフセット行列も保存

				m_boneInfoMap[boneName] = newBoneInfo;
				boneID = m_boneCounter;
				m_boneCounter++; // カウンターを進める
			}
			else
			{
				// 既に登録済みならそのIDを使う（パーツが分かれているモデル対策）
				boneID = m_boneInfoMap[boneName].id;
			}

			for (unsigned int w = 0; w < bone->mNumWeights; w++)
			{
				unsigned int vertexId = bone->mWeights[w].mVertexId;
				float weight = bone->mWeights[w].mWeight;

				if (vertexId >= vertices.size()) continue;

				int slot = weightCounts[vertexId];
				if (slot < 4) // 最大4つまで
				{
					// ループの b ではなく、重複防止した boneID を割り当てる
					vertices[vertexId].boneIndices[slot] = boneID;
					vertices[vertexId].boneWeights[slot] = weight;
					weightCounts[vertexId]++;
				}
			}
		}

		// ウェイトの合計が1.0になるように綺麗に整える
		for (auto& v : vertices)
		{
			float totalWeight = v.boneWeights[0] + v.boneWeights[1] + v.boneWeights[2] + v.boneWeights[3];
			if (totalWeight > 0.0f)
			{
				v.boneWeights[0] /= totalWeight;
				v.boneWeights[1] /= totalWeight;
				v.boneWeights[2] /= totalWeight;
				v.boneWeights[3] /= totalWeight;
			}
			else
			{
				// どの骨からもウェイトが割り振られなかった場合
				v.boneWeights[0] = 1.0f;
				v.boneIndices[0] = 0;
			}
		}

		// インデックスデータの抽出
		for (unsigned int f = 0; f < ai_mesh->mNumFaces; f++)
		{
			aiFace face = ai_mesh->mFaces[f];
			for (unsigned int ind = 0; ind < face.mNumIndices; ind++)
			{
				indices.push_back(face.mIndices[ind]);
			}
		}

		// 抽出したデータからMeshを1つ作成し、リストに追加
		auto newMesh = std::make_shared<Mesh>();
		if (newMesh->Create(device, vertices, indices))
		{
			m_meshes.push_back(newMesh);
		}
	}

	// ルートノード（一番大元の骨）から階層構造をすべて読み取る
	ReadNodeHierarchy(m_rootNode, scene->mRootNode);

	// アニメーションデータの読み込み
	if (scene->HasAnimations())
	{
		// FBXの中にアニメーションが入っていれば、一つ目のアニメーションを読み込む
		auto animation = std::make_shared<Animation>();
		if (animation->Load(scene->mAnimations[0]))
		{

			m_animation = animation; // 読み込み成功したら保存する
			OutputDebugStringA("アニメーションの読み込みに成功しました。アニメーションを保存します。");
		}
	}
	
	return true;
}

void Model::Draw(ID3D11DeviceContext* context)
{
	// 持っているパーツを順番に描画
	for (auto& mesh : m_meshes)
	{
		mesh->Bind(context);
		mesh->Draw(context);
	}
}

// 骨の親子関係（ノード階層）を再帰的に読み取って保存する関数
void Model::ReadNodeHierarchy(AssimpNodeData& dest, const aiNode* src)
{
	dest.name = src->mName.data;
	dest.transformation = ConvertMatrixToDirectXFormat(src->mTransformation); // 初期状態の行列
	dest.children.resize(src->mNumChildren);

	// 自分の子供たちも順番に読み取っていく
	for (unsigned int i = 0; i < src->mNumChildren; i++)
	{
		ReadNodeHierarchy(dest.children[i], src->mChildren[i]);
	}
}
