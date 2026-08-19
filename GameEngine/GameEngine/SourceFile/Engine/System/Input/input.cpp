#include "Engine/System/Input/Input.h"

// 配列の初期化
bool Input::s_currentKeys[256] = {};
bool Input::s_prevKeys[256] = {};


void Input::Intialize()
{
	for (int i = 0; i < 256; i++)
	{
		s_currentKeys[i] = false;
		s_prevKeys[i] = false;
	}
}

void Input::Update()
{
	for (int i = 0; i < 256; i++)
	{
		// 現在の状況と1フレーム前の状況として保存する
		s_prevKeys[i] = s_currentKeys[i];

		s_currentKeys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
	}
}
// 押され続けているか
bool Input::GetKey(KeyCode key)// 押され続けているか
{
	return s_currentKeys[static_cast<int>(key)];
}

bool Input::GetKeyDown(KeyCode key) // 押した瞬間か
{
	return s_currentKeys[static_cast<int>(key)];
}

bool Input::GetKeyUp(KeyCode key)
{
	return !s_currentKeys[static_cast<int>(key)] && s_prevKeys[static_cast<int>(key)];
}

