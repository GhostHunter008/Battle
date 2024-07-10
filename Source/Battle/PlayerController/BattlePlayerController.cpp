// Fill out your copyright notice in the Description page of Project Settings.


#include "BattlePlayerController.h"
#include "Battle/HUD/BattleHUD.h"
#include "Battle/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Battle/Character/BattleCharacter.h"
#include "Battle/GameMode/BattleGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Battle/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "Battle/BattleComponents/CombatComponent.h"
#include "Battle/GameState/BattleGameState.h"
#include "Battle/PlayerState/BattlePlayerState.h"
#include "GameFramework/PlayerState.h"
#include "Components/Image.h"

void ABattlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	BattleHUD=Cast<ABattleHUD>(GetHUD());
	ServerCheckMatchState();
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
	CheckPing(DeltaTime); 
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
		bInitializeHealth = true;
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
		bInitializeScore = true;
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
		bInitializeDefeats = true;
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
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
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
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ABattlePlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) 
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	else if(MatchState == MatchState::Cooldown)
	{
		TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		BattleGameMode=BattleGameMode==nullptr?Cast<ABattleGameMode>(UGameplayStatics::GetGameMode(GetWorld())) : BattleGameMode;
		if (BattleGameMode)
		{
			SecondsLeft = FMath::CeilToInt(BattleGameMode->GetCountdownTime()) + LevelStartingTime;
			//GEngine->AddOnScreenDebugMessage(-1,1,FColor::Red, FString::Printf(TEXT("Remaining Time: %f"),BattleGameMode->GetCountdownTime()));
		}
	}
	
	if (CountdownInt != SecondsLeft) // 每秒更新一次
	{
		if (MatchState == MatchState::WaitingToStart)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountDownTime(TimeLeft);
		} 
		else if (MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;
}

void ABattlePlayerController::SetHUDMatchCountDownTime(float CountDownTime)
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	bool bHUDValid = BattleHUD &&
		BattleHUD->CharacterOverlay &&
		BattleHUD->CharacterOverlay->MatchCountDownText;
	if (bHUDValid)
	{
		if (CountDownTime < 0.f)
		{
			BattleHUD->CharacterOverlay->MatchCountDownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountDownTime / 60.f);
		int32 Seconds = CountDownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BattleHUD->CharacterOverlay->MatchCountDownText->SetText(FText::FromString(CountdownText));
	}
}

void ABattlePlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	bool bHUDValid = BattleHUD &&
		BattleHUD->Announcement &&
		BattleHUD->Announcement->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BattleHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BattleHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABattlePlayerController::SetHUDGrenades(int32 Grenades)
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	bool bHUDValid = BattleHUD &&
		BattleHUD->CharacterOverlay &&
		BattleHUD->CharacterOverlay->GrenadeAmountText;
	if (bHUDValid)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		BattleHUD->CharacterOverlay->GrenadeAmountText->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABattlePlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	bool bHUDValid = BattleHUD &&
		BattleHUD->CharacterOverlay &&
		BattleHUD->CharacterOverlay->ShieldBar &&
		BattleHUD->CharacterOverlay->ShieldText;
	if (bHUDValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		BattleHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BattleHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
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
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo) SetHUDCarryAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);

				ABattleCharacter* BattleCharacter = Cast<ABattleCharacter>(GetPawn());
				if (BattleCharacter && BattleCharacter->GetCombat())
				{
					if (bInitializeGrenades) SetHUDGrenades(BattleCharacter->GetCombat()->GetGrenadeAmount());
				}
			}
		}
	}
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


void ABattlePlayerController::ServerCheckMatchState_Implementation()
{
	ABattleGameMode* GameMode = Cast<ABattleGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		CooldownTime=GameMode->CooldownTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime,CooldownTime, LevelStartingTime);
	}
}

void ABattlePlayerController::ClientJoinMidgame_Implementation(FName InMatchState, float InWarmupTime, float InMatchTime, float InCooldownTime, float InLevelStartingTime)
{
	WarmupTime = InWarmupTime;
	MatchTime = InMatchTime;
	CooldownTime=InCooldownTime;
	LevelStartingTime = InLevelStartingTime;
	MatchState = InMatchState;
	OnMatchStateSet(MatchState);

	if (BattleHUD && MatchState == MatchState::WaitingToStart)
	{
		BattleHUD->AddAnnouncement();
	}
}

void ABattlePlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABattlePlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABattlePlayerController::HandleMatchHasStarted()
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	if (BattleHUD)
	{
		if (BattleHUD->CharacterOverlay == nullptr)
		{
			BattleHUD->AddCharacterOverlay();
		}		
		if (BattleHUD->Announcement)
		{
			BattleHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABattlePlayerController::HandleCooldown()
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	if (BattleHUD)
	{
		BattleHUD->CharacterOverlay->RemoveFromParent();
		bool bHUDValid = BattleHUD->Announcement &&
			BattleHUD->Announcement->AnnouncementText &&
			BattleHUD->Announcement->InfoText;
		if (bHUDValid)
		{
			BattleHUD->Announcement->SetVisibility(ESlateVisibility::Visible);

			FString AnnouncementText("New Match Starts In:");
			BattleHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ABattleGameState* BattleGameState = Cast<ABattleGameState>(UGameplayStatics::GetGameState(this));
			ABattlePlayerState* BattlePlayerState = GetPlayerState<ABattlePlayerState>();
			if (BattleGameState && BattlePlayerState)
			{
				TArray<ABattlePlayerState*> TopPlayers = BattleGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("There is no winner.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == BattlePlayerState)
				{
					InfoTextString = FString("You are the winner!");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players tied for the win:\n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}

				BattleHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
		BattleHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
	}

	ABattleCharacter* BattleCharacter = Cast<ABattleCharacter>(GetPawn());
	if (BattleCharacter)
	{
		BattleCharacter->bDisableGameplay = true;
		if (BattleCharacter->GetCombat())
		{
			BattleCharacter->GetCombat()->FireButtonPress(false);
		}
	}
}

void ABattlePlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
		if (PlayerState)
		{
			//if (PlayerState->GetCompressedPing() * 4 > HighPingThreshold) // ping is compressed; it's actually ping / 4
			if (PlayerState->GetPingInMilliseconds()> HighPingThreshold)
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
			}
		}
		HighPingRunningTime = 0.f;
	}
	bool bHighPingAnimationPlaying =
		BattleHUD && BattleHUD->CharacterOverlay &&
		BattleHUD->CharacterOverlay->HighPingAnimation &&
		BattleHUD->CharacterOverlay->IsAnimationPlaying(BattleHUD->CharacterOverlay->HighPingAnimation);
	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void ABattlePlayerController::HighPingWarning()
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	bool bHUDValid = BattleHUD &&
		BattleHUD->CharacterOverlay &&
		BattleHUD->CharacterOverlay->HighPingImage &&
		BattleHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		BattleHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		BattleHUD->CharacterOverlay->PlayAnimation(
			BattleHUD->CharacterOverlay->HighPingAnimation,
			0.f, // 播放时间
			5);  // 循环次数
	}
}

void ABattlePlayerController::StopHighPingWarning()
{
	BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(GetHUD()) : BattleHUD;
	bool bHUDValid = BattleHUD &&
		BattleHUD->CharacterOverlay &&
		BattleHUD->CharacterOverlay->HighPingImage &&
		BattleHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		BattleHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (BattleHUD->CharacterOverlay->IsAnimationPlaying(BattleHUD->CharacterOverlay->HighPingAnimation))
		{
			BattleHUD->CharacterOverlay->StopAnimation(BattleHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}
