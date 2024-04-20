// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BattleGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BATTLE_API ABattleGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PlayerEliminated(class ABattleCharacter* ElimmedCharacter, class ABattlePlayerController* VictimController, ABattlePlayerController* AttackerController);
	
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
};
