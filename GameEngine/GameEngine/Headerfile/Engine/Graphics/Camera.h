#pragma once
#include <DirectXMath.h>

///////////////-----メモ-----//////////////////////////////////////
//
//・DirectX::XMMatrixLookAtLH：カメラを世界の中心（原点）に固定し、
//                             宇宙全体を逆方向に動かす役割
// 
// ・DirectX::XMMatrixPerspectiveFovLH：遠くのものを小さくし、
//                                      画面（2D）に押し込むための枠を作る役割
// 
//////////////////////////////////////////////////////////////////


class Camera
{
public:
	Camera();
	~Camera();

	// カメラの位置と向いている方向を設定
	// position (Eye) : カメラの現在の位置
	// target (Focus) : カメラが「何を見つめているか」の中心点
	// up (Up) : カメラにとって「どっちが上か」これがないと、カメラが横に傾いているか、逆立ちしているか分からない
	void SetLookAt(DirectX::XMVECTOR position, DirectX::XMVECTOR target, DirectX::XMVECTOR up);

	
	// レンズ（画角やアスペクト比）を設定
	// foreAngle (Field of View) : 画角　大きい数字にすると周囲が広く見えるが、端が歪む
	// aspectRatio (画面の横幅 / 縦幅) : アスペクト比
	// nearZ,farZ : 描画する限界の距離
	void SetPerspective(float fovAngle, float aspectRatio, float nearZ, float farZ);

	// 計算済みの行列を取得する
	DirectX::XMMATRIX GetViewMatrix() const
	{
		return m_viewMatrix;
	}

	// 計算済みの行列を取得する
	DirectX::XMMATRIX GetProjectionMatrix() const
	{
		return m_projectionMatrix;
	}

private:
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projectionMatrix;
};
