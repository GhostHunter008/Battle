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
		ElimmedCharacter->Reset(); // ��controller��pawn����
		ElimmedCharacter->Destroy(); // �����Զ�ͬ�����ͻ���
	}
	if (ElimmedController)
	{
		// �漴�ڳ�����ѡ��һ��λ��
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
