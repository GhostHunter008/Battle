// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BATTLE_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	
	// 当有玩家进入时自动调用该函数
	virtual void PostLogin(APlayerController* NewPlayer) override;

	
};
