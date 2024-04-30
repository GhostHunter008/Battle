// Fill out your copyright notice in the Description page of Project Settings.


#include "BattleGameMode.h"
#include "Battle/Character/BattleCharacter.h"
#include "Battle/PlayerController/BattlePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Battle/PlayerState/BattlePlayerState.h"

ABattleGameMode::ABattleGameMode()
{
	bDelayedStart=true; // match state 停留在Waiting start 的状态，此时default pawn尚未生成，玩家可以在地图中飞行
}

void ABattleGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABattleGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
}

void ABattleGameMode::PlayerEliminated(class ABattleCharacter* ElimmedCharacter, class ABattlePlayerController* VictimController, ABattlePlayerController* AttackerController)
{
	ABattlePlayerState* AttackerPlayerState = AttackerController ? Cast<ABattlePlayerState>(AttackerController->PlayerState) : nullptr;
	ABattlePlayerState* VictimPlayerState = VictimController ? Cast<ABattlePlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(5);
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

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

void ABattleGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABattlePlayerController* BattlePlayer = Cast<ABattlePlayerController>(*It);
		if (BattlePlayer)
		{
			BattlePlayer->OnMatchStateSet(MatchState);
		}
	}
}
