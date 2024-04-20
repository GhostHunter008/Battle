// Fill out your copyright notice in the Description page of Project Settings.


#include "BattlePlayerState.h"
#include "Battle/Character/BattleCharacter.h"
#include "Battle/PlayerController/BattlePlayerController.h"

void ABattlePlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	BattleCharacter = BattleCharacter == nullptr ? Cast<ABattleCharacter>(GetPawn()) : BattleCharacter;
	if (BattleCharacter)
	{
		BattlePlayerController = BattlePlayerController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->GetController()) : BattlePlayerController;
		if (BattlePlayerController)
		{
			
		}
	}
	


}

void ABattlePlayerState::AddToScore(float ScoreAmount)
{
	Score+=ScoreAmount;

	BattleCharacter = BattleCharacter == nullptr ? Cast<ABattleCharacter>(GetPawn()) : BattleCharacter;
	if (BattleCharacter)
	{
		BattlePlayerController = BattlePlayerController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->GetController()) : BattlePlayerController;
		if (BattlePlayerController)
		{
			
		}
	}
}
