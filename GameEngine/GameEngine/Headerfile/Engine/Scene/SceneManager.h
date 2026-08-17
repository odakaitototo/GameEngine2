#pragma once
#include <string>

// Applicationへのポインタを使うための前方宣言
class Application;

class SceneManager
{
public:
	// どこからでも SceneManager::LoadScene()と呼べるようにstaticする
	static void SaveScene(Application* app, const std::string& filename);
	static void LoadScene(Application* app, const std::string& filename);
};
