#pragma
#include "Engine/Component/ScriptComponent.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Core/Application.h"
class SkyDomeController : public ScriptComponent
{
public:
	std::string GetScriptName() const override
	{
		return "SkyDomeController";
	}

protected:
	void OnUpdate() override
	{
		auto& transform = gameObject->GetTransform();
		auto targetPos = Application::GetInstance()->GetGameCamera().GetTarget();
	}
};
