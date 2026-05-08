#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h> // 数字ライブラリ（座標計算用）



// Transform情報をまとめる構造体
struct Transform
{
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f }; // 位置
	DirectX::XMFLOAT3 rotation = { 0.0f,0.0f,0.0f }; // 回転
	DirectX::XMFLOAT3 scale = { 1.0f,1.0f,1.0f }; // 拡大縮小
};

class GameObject
{
public:
	GameObject(std::string name) : m_name(name){}
	~GameObject(){}

	// ゲッター・セッター
	const std::string& GetName() const { return m_name; }
	void SetName(const std::string& name) { m_name = name; }

	Transform& GetTransform() { return m_transform; }


private:
	std::string m_name; // オブジェクト名
	Transform m_transform; // 位置・回転・スケール
};