#pragma once
#include "Engine/Component/ScriptComponent.h"




class TestController : public ScriptComponent
{
public:
	// ©•ª‚ª‚Â‚¯‚½‚¢Script–¼‚ğ•t‚¯‚é
	std::string GetScriptName() const override;

	// –ˆƒtƒŒ[ƒ€ŒÄ‚Î‚ê‚éˆ—
	void Update() override;
	
};