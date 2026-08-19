#include "../Headerfile/Game/Script/EngineTestScriipt/TestController.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/System/Input/Input.h"
#include "Engine/System/Time/Time.h"

std::string TestController::GetScriptName() const
{
	return "TestController";
}

void TestController::Update()
{
	// 自分がアタッチされているオブジェクトのTransformを取得
	auto& transform = gameObject->GetTransform();

	float deltTime = Time::GetDeltaTime(); // 歩くスピードを1秒1メートルにする
	float speed = 5.0f; // 歩くスピード

	//WASDで移動
	if (Input::GetKey(KeyCode::W))
	{
		transform.position.z += speed * deltTime;
	}

	if (Input::GetKey(KeyCode::S))
	{
		transform.position.z -= speed * deltTime;
	}

	if (Input::GetKey(KeyCode::A))
	{
		transform.position.x -= speed * deltTime;
	}

	if (Input::GetKey(KeyCode::D))
	{
		transform.position.x += speed * deltTime;
	}
}