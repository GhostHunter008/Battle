// Fill out your copyright notice in the Description page of Project Settings.


#include "BattlePlayerController.h"
#include "Battle/HUD/BattleHUD.h"
#include "Battle/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Battle/Character/BattleCharacter.h"
#include "Battle/GameMode/BattleGameMode.h"
#include "Net/UnrealNetwork.h"

void ABattlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	BattleHUD=Cast<ABattleHUD>(GetHUD());
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

void ABattlePlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABattlePlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABattlePlayerController, MatchState);
}

void ABattlePlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime(); // 绘制时间
	CheckTimeSync(DeltaTime); // 每隔一段时间重新同步一次，即设置正确的ClientServerDelta
	PollInit(); // 轮询，直到HUD被创建（这种实现并不好）
}

void ABattlePlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
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
	else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}

}

void ABattlePlayerController::SetHUDScore(float Score)
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	bool bHUDValid = BattleHUD &&
		BattleHUD->CharacterOverlay &&
		BattleHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText=FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BattleHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}
}

void ABattlePlayerController::SetHUDDefeats(int32 Defeats)
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	bool bHUDValid = BattleHUD &&
		BattleHUD->CharacterOverlay &&
		BattleHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BattleHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void ABattlePlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	bool bHUDValid = BattleHUD &&
		BattleHUD->CharacterOverlay &&
		BattleHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString WeaponAmmoText = FString::Printf(TEXT("%d"), Ammo);
		BattleHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(WeaponAmmoText));
	}
}

void ABattlePlayerController::SetHUDCarryAmmo(int32 Ammo)
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	bool bHUDValid = BattleHUD &&
		BattleHUD->CharacterOverlay &&
		BattleHUD->CharacterOverlay->CarryAmmoAmount;
	if (bHUDValid)
	{
		FString CarryAmmoText = FString::Printf(TEXT("%d"), Ammo);
		BattleHUD->CharacterOverlay->CarryAmmoAmount->SetText(FText::FromString(CarryAmmoText));
	}
}

void ABattlePlayerController::SetHUDMatchCountDownTime(float CountDownTime)
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	bool bHUDValid = BattleHUD &&
		BattleHUD->CharacterOverlay &&
		BattleHUD->CharacterOverlay->MatchCountDownText;
	if (bHUDValid)
	{
		int32 Minutes = FMath::FloorToInt(CountDownTime / 60.f);
		int32 Seconds = CountDownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BattleHUD->CharacterOverlay->MatchCountDownText->SetText(FText::FromString(CountdownText));
	}
}

void ABattlePlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (BattleHUD && BattleHUD->CharacterOverlay)
		{
			CharacterOverlay = BattleHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
}

void ABattlePlayerController::SetHUDTime()
{
	uint32 SecondsLeft = FMath::CeilToInt(MatchTime - GetServerTime()); // GetWorld()->GetTimeSeconds()对每个机器实例都不相同
	if (CountdownInt != SecondsLeft) // 每秒更新一次
	{
		SetHUDMatchCountDownTime(MatchTime - GetServerTime());
	} 

	CountdownInt = SecondsLeft;
}

void ABattlePlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABattlePlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABattlePlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}


void ABattlePlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}


void ABattlePlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
		if (BattleHUD)
		{
			BattleHUD->AddCharacterOverlay();
		}
	}
}

void ABattlePlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
		if (BattleHUD)
		{
			BattleHUD->AddCharacterOverlay();
		}
	}
}
