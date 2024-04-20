// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BattlePlayerState.generated.h"


UCLASS()
class BATTLE_API ABattlePlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	virtual void OnRep_Score() override; // 只会在客户端执行
	void AddToScore(float ScoreAmount);

private:
	class ABattleCharacter* BattleCharacter;
	class ABattlePlayerController* BattlePlayerController;
	
};
