#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Battle/HUD/BattleHUD.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BATTLE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	friend class ABattleCharacter; // 强关联性
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	

	void EquipWeapon(class AWeapon* InWeapon);

protected:
	virtual void BeginPlay() override;

	void SetAiming(bool InbAiming);
	UFUNCTION(Server,Reliable)
	void ServerSetAiming(bool InbAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void FireButtonPress(bool bPressed);

	void Fire();

	UFUNCTION(Server,Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget); // FVector_NetQuantize 网络传输优化的结构体

	UFUNCTION(NetMulticast,Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	// 以十字准线为基准，进行射线检测
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	// 设置扩散准星
	void SetHUDCrosshairs(float DeltaTime);

private:
	class ABattleCharacter* BattleCharacter;
	class ABattlePlayerController* BattleController;
	class ABattleHUD* BattleHUD;

	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon)
	class AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	// hud and crosshairs
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	// 武器旋转矫正时会用到
	FVector HitTarget;

	FHUDPackage HUDPackage;

	/*
	*  Aiming and FOV
	*/

	float DefaultFOV; // 由玩家初始的相机FOV决定

	float CurrentFOV; // 当前视野
	
	UPROPERTY(EditAnywhere,Category=Combat)
	float ZoomedFOV=30.f; 

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f; // 玩家收枪后视野变化速度，无论什么枪都是一样的
	
	void InterpFOV(float DeltaTime);

	/*Auto Fire*/
	FTimerHandle FireTimer;
	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();

		
};
