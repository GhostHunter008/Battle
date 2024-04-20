// Fill out your copyright notice in the Description page of Project Settings.


#include "BattlePlayerController.h"
#include "Battle/HUD/BattleHUD.h"
#include "Battle/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Battle/Character/BattleCharacter.h"

void ABattlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	BattleHUD=Cast<ABattleHUD>(GetHUD());
}

void ABattlePlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BattleHUD = BattleHUD==nullptr ? Cast<ABattleHUD>(GetHUD()):BattleHUD;
	bool bHUDValid = BattleHUD &&
		BattleHUD->CharacterOverlay &&
		BattleHUD->CharacterOverlay->HealthBar &&
		BattleHUD->CharacterOverlay->HealthText;
	if (bHUDValid) // TODO:这里的实现有待改进，不符合迪米特法则，应该调用character中的SetHealth函数
	{
		const float HealthPercent = Health / MaxHealth;
		BattleHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BattleHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}

}

void ABattlePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABattleCharacter* BattleCharacter = Cast<ABattleCharacter>(InPawn);
	if (BattleCharacter)
	{
		SetHUDHealth(BattleCharacter->GetHealth(), BattleCharacter->GetMaxHealth());
	}
}
