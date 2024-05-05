// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BattleGameMode.generated.h"

// 声明，仿照Gamemode
namespace MatchState
{
	extern BATTLE_API const FName Cooldown; // Match duration has been reached. Display winner and begin cooldown timer.
}

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
	float LevelStartingTime = 0.f; // GetWorld()->GetTimeSeconds()是从刚进入map时就开始计算的。

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;  // 热身时间

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;  // 比赛时间

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f; // 冷却时间，用于显示玩家排名等

	float CountdownTime = 0.f; // 辅助变量
	FORCEINLINE float GetCountdownTime(){return CountdownTime;}
	

};
