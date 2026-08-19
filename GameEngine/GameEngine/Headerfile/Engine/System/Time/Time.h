#pragma once

#include <Windows.h>

// 時間管理システム
class Time
{
public:
	static void Initialize(); // エンジン起動時の初期化
	static void Update(); // 毎フレームの更新

	// どこからでも「1フレームの経過時間(秒)」を取得できるようにする関数
	static float GetDeltaTime();

private:
	static float s_deltaTime; // 経過時間(秒)
	static LARGE_INTEGER s_timeStart; // 計測開始時間
	static LARGE_INTEGER s_timeFreq; // PCのタイマーの周波数
};


