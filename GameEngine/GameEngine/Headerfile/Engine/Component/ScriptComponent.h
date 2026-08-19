#pragma once
#include "Engine/Component/ComponentBase.h"
#include <string>

// 前方宣言
class ColliderBase;

// ゲームのルールを書くための親クラス
class ScriptComponent : public ComponentBase
{
public:
	virtual ~ScriptComponent() = default;

	virtual void Start() // Startモードになった時に1度だけ呼ばれる
	{

	}

	virtual void Update() override // Playモードの時に毎フレーム呼ばれる
	{

	}

	virtual void OnTriggerStay(ColliderBase* other)
	{

	}

	virtual std::string GetScriptName() const // 自分のクラス名を文字列で返す関数
	{
		return "UnknownScript";
	}
};