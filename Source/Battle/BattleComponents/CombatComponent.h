#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

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

	UFUNCTION(Server,Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast,Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

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

	bool FireButtonPressed;

	// hud and crosshairs
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	
	

		
};
