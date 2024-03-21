// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameState.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (GameState)
	{
		int32 NumOfPlayers = GameState->PlayerArray.Num();
		if (NumOfPlayers == 2) // TODO：做成一个公开的可变变量
		{
			UWorld* World = GetWorld();
			if (World)
			{
				bUseSeamlessTravel=true; // 使用SeamlessTravel模式
				World->ServerTravel(FString("/Game/Maps/BattleMap?listen")); // TODO：做成一个可在蓝图配置的字符串
			}
		}
	}
}
