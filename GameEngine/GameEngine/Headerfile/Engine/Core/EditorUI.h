#pragma once

// 前方宣言
class Application;

class EditorUI
{
public:
	EditorUI() = default;
	~EditorUI() = default;

	// UI描画処理：Applicationの「ポインタ」を受け取って操作する
	void Draw(Application* app);
};
