#pragma once

class Application;

class Renderer
{
public:
	Renderer() = default;
	~Renderer() = default;

	void Render(Application* app);
};