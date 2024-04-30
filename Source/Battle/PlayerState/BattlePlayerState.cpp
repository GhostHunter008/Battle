// Fill out your copyright notice in the Description page of Project Settings.


#include "BattlePlayerState.h"
#include "Battle/Character/BattleCharacter.h"
#include "Battle/PlayerController/BattlePlayerController.h"
#include "Net/UnrealNetwork.h"

void ABattlePlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABattlePlayerState,Defeats);
}

void ABattlePlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	BattleCharacter = BattleCharacter == nullptr ? Cast<ABattleCharacter>(GetPawn()) : BattleCharacter;
	if (BattleCharacter)
	{
		BattlePlayerController = BattlePlayerController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->GetController()) : BattlePlayerController;
		if (BattlePlayerController)
		{
			BattlePlayerController->SetHUDScore(GetScore());
		}
	}
}

void ABattlePlayerState::AddToScore(float ScoreAmount)
{
	SetScore(ScoreAmount+ GetScore());

	BattleCharacter = BattleCharacter == nullptr ? Cast<ABattleCharacter>(GetPawn()) : BattleCharacter;
	if (BattleCharacter)
	{
		BattlePlayerController = BattlePlayerController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->GetController()) : BattlePlayerController;
		if (BattlePlayerController)
		{
			BattlePlayerController->SetHUDScore(GetScore());
		}
	}
}

void ABattlePlayerState::OnRep_Defeats()
{
	BattleCharacter = BattleCharacter == nullptr ? Cast<ABattleCharacter>(GetPawn()) : BattleCharacter;
	if (BattleCharacter)
	{
		BattlePlayerController = BattlePlayerController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->GetController()) : BattlePlayerController;
		if (BattlePlayerController)
		{
			BattlePlayerController->SetHUDScore(Defeats);
		}
	}
}

void ABattlePlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats+=DefeatsAmount;

	BattleCharacter = BattleCharacter == nullptr ? Cast<ABattleCharacter>(GetPawn()) : BattleCharacter;
	if (BattleCharacter)
	{
		BattlePlayerController = BattlePlayerController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->GetController()) : BattlePlayerController;
		if (BattlePlayerController)
		{
			BattlePlayerController->SetHUDDefeats(Defeats);
		}
	}
}
