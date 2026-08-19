#pragma once
#include <Windows.h>

// キーボードコード
enum class KeyCode
{
	None = 0,
// アルファベット
W = 'W', A = 'A', S = 'S', D = 'D',
Q = 'Q', E = 'E', R = 'R', I = 'I', J = 'J', K = 'K', L = 'L', U = 'U', O = 'O',
T = 'T', Y = 'Y', P = 'P', F = 'F', G = 'G', H = 'H', Z = 'Z', X = 'X',
C = 'C', V = 'V', B = 'B', N = 'N', M = 'M', 

// 特殊キー
Up = VK_UP, Down = VK_DOWN, Left = VK_LEFT, Right = VK_RIGHT,
Space = VK_SPACE, Enter = VK_RETURN, Escape = VK_ESCAPE,
Shift = VK_SHIFT, Ctrl = VK_CONTROL,
};


// 入力管理システム
class Input
{
public:
	static void Intialize(); // 初期化
	static void Update(); // 毎フレーム更新


	// 指定したキーが押されている間反応します
	// 使い方：GetKey(KeyCode::W)
	static bool GetKey(KeyCode Key);


	
	// 指定したキーが押された瞬間（押下が開始されたフレーム）かどうかを判定します。
	// 使い方：GetKeyDown(KeyCode::W)
	static bool GetKeyDown(KeyCode key);


	
	// 指定したキーが離されたかどうかを取得します。
	// 使い方：GetKeyUp(KeyCode::W)
	static bool GetKeyUp(KeyCode key);

private:
	// キーの現在の状態と1フレーム前の状態を記憶する配列
	static bool s_currentKeys[256];
	static bool s_prevKeys[256];


};