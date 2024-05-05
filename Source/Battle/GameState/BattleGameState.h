// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BattleGameState.generated.h"

UCLASS()
class BATTLE_API ABattleGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


public:
	void UpdateTopScore(class ABattlePlayerState* ScoringPlayer); // 负责更新TopScoringPlayers

	UPROPERTY(Replicated)
	TArray<ABattlePlayerState*> TopScoringPlayers; // 时刻存储着最高分球员的playerstate

	float TopScore = 0.f; // 辅助变量
};
