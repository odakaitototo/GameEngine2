#pragma once
#include <vector>
#include <memory>
#include "Engine//Scene/GameObject.h"

class RigidbodySystem
{
public:
	void Update(std::vector<std::shared_ptr<GameObject>>& gameobjects);
};