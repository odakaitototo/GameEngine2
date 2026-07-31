#pragma once

#include "Engine/Core/DebugRenderer.h"

class Application;

class Renderer
{
public:
	Renderer() = default;
	~Renderer() = default;

	void Render(Application* app);

private:

	// ƒfƒoƒbƒO•`‰æ‚ð’S“–‚·‚é‚à‚Ì
	DebugRenderer m_debugRenderer;
};