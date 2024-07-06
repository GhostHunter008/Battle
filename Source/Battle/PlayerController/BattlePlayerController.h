#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BattlePlayerController.generated.h"


UCLASS()
class BATTLE_API ABattlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void ReceivedPlayer() override; // Sync with server clock as soon as possible
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);

	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarryAmmo(int32 Ammo);

	void SetHUDTime();
	void SetHUDMatchCountDownTime(float CountDownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);

	void SetHUDGrenades(int32 Grenades);

/************************************************************************/
/*  初始化HUD
/************************************************************************/
	
	void PollInit(); // 轮询，值得改进
	bool bInitializeCharacterOverlay = false;

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeats;
	int32 HUDGrenades;

/**
* Sync time between client and server
*/

	virtual float GetServerTime(); // Synced with server world clock
	
	// Requests the current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f; // difference between client and server time

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName InMatchState, float InWarmupTime, float InMatchTime,float InCooldownTime, float InLevelStartingTime);

	float LevelStartingTime = 0.f; // 启动到真正开始阶段的时间，一服务器上的时间为准
	float WarmupTime = 0.f; // 热身时间，从gamemode中获取
	float MatchTime = 0.f; // 比赛时间，从gamemode中获取
	float CooldownTime = 0.f; // 冷却时间，从gamemode中获取

	uint32 CountdownInt = 0; // 辅助变量，用于达到每秒更新的目的

/************************************************************************/
/*  Match State                                                 
/************************************************************************/

	void OnMatchStateSet(FName State);

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	void HandleMatchHasStarted();
	void HandleCooldown();


private:
	class ABattleHUD* BattleHUD;
	class UCharacterOverlay* CharacterOverlay;
	class ABattleGameMode* BattleGameMode;




};
