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
	ABattleGameMode();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

public:
	virtual void PlayerEliminated(class ABattleCharacter* ElimmedCharacter, class ABattlePlayerController* VictimController, ABattlePlayerController* AttackerController);
	
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);

	virtual void OnMatchStateSet() override;

public:
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	float CountdownTime = 0.f;

	float LevelStartingTime = 0.f; // GetWorld()->GetTimeSeconds()是从刚进入map时就开始计算的。

};
