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
		if (NumOfPlayers == 2) // TODO
		{
			UWorld* World = GetWorld();
			if (World)
			{
				bUseSeamlessTravel=true;
				World->ServerTravel(FString("/Game/Maps/BattleMap?listen"));
			}
		}
	}
}
