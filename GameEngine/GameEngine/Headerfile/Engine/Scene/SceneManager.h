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

	static void ExecuteLoadScene(Application* app);

	static void LoadWithLoadingScreen(Application* app, const std::string& targetFilename);
	static std::string s_targetScene; // 本当に遷移したいシーン

};
