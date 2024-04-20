// Fill out your copyright notice in the Description page of Project Settings.


#include "BattleGameMode.h"
#include "Battle/Character/BattleCharacter.h"
#include "Battle/PlayerController/BattlePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

void ABattleGameMode::PlayerEliminated(class ABattleCharacter* ElimmedCharacter, class ABattlePlayerController* VictimController, ABattlePlayerController* AttackerController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void ABattleGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset(); // 将controller和pawn分离
		ElimmedCharacter->Destroy(); // 销毁自动同步至客户端
	}
	if (ElimmedController)
	{
		// 随即在出生点选择一个位置
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
