#include "Engine/System/Time/Time.h"


// static変数の実態の定義
float Time::s_deltaTime = 0.0f;
LARGE_INTEGER Time::s_timeStart = {};
LARGE_INTEGER Time::s_timeFreq = {};

void Time::Initialize()
{
	// PCの内臓タイマーの性能をチェック
	QueryPerformanceFrequency(&s_timeFreq);

	// 最初の時間を記録
	QueryPerformanceCounter(&s_timeStart);
}

void Time::Update()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	// 前回のフレームからの経過時間を計算（秒単位に変換）
	s_deltaTime = static_cast<float>(currentTime.QuadPart - s_timeStart.QuadPart) / s_timeFreq.QuadPart;

	// 今回の時間を「前回」として保存し直す
	s_timeStart = currentTime;

	if (s_deltaTime > 0.1f)
	{
		s_deltaTime = 0.1f;
	}
}

float Time::GetDeltaTime()
{
	return s_deltaTime;
}