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
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

// Score 已经处理好
public:
	virtual void OnRep_Score() override; // 只会在客户端执行
	void AddToScore(float ScoreAmount); // 服务端调用

// Defeats 自己处理
public:
	UPROPERTY(ReplicatedUsing= OnRep_Defeats)
	int32 Defeats;

	UFUNCTION()
	virtual void OnRep_Defeats();
	void AddToDefeats(int32 DefeatsAmount);

private:
	class ABattleCharacter* BattleCharacter;
	class ABattlePlayerController* BattlePlayerController;
	
};
