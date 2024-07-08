#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Battle/HUD/BattleHUD.h"
#include "Battle/Weapon/WeaponTypes.h"
#include "Battle/BattleTypes/CombatStates.h"
#include "CombatComponent.generated.h"


// #define TRACE_LENGTH 80000; 在WeaponType中定义


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BATTLE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

/************************************************************************/
/*  组件自身属性相关
/************************************************************************/
public:	
	friend class ABattleCharacter; // 强关联性
	UCombatComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

private:
	class ABattleCharacter* BattleCharacter;
	class ABattlePlayerController* BattleController;
	class ABattleHUD* BattleHUD;
	
/************************************************************************/
/* 装备武器                                                    
/************************************************************************/
public:
	void EquipWeapon(class AWeapon* InWeapon);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	class AWeapon* EquippedWeapon;

	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);

	void DropEquippedWeapon();

	/**
	 * 副武器
	 */
	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;

	UFUNCTION()
	void OnRep_SecondaryWeapon();

	void AttachActorToBackpack(AActor* ActorToAttach);

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);

	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

	void SwapWeapons();

	bool ShouldSwapWeapons();

/************************************************************************/
/* 瞄准                                                 
/************************************************************************/

	void SetAiming(bool InbAiming);
	UFUNCTION(Server,Reliable)
	void ServerSetAiming(bool InbAiming);

	UPROPERTY(Replicated)
	bool bAiming;

	// 以十字准线为基准，进行射线检测
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	// 武器旋转矫正时会用到
	FVector HitTarget;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	// Aiming and FOV
	float DefaultFOV; // 由玩家初始的相机FOV决定
	float CurrentFOV; // 当前视野

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f; // 玩家收枪后视野变化速度，无论什么枪都是一样的；开枪的速度由武器决定

	void InterpFOV(float DeltaTime);


/************************************************************************/
/* 开火                                                            
/************************************************************************/
	void FireButtonPress(bool bPressed);

	bool bFireButtonPressed;

	void Fire();

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget); // FVector_NetQuantize 网络传输优化的结构体

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	// Auto Fire
	FTimerHandle FireTimer;
	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();

/************************************************************************/
/* 准星扩散
/************************************************************************/

	// 设置扩散准星
	void SetHUDCrosshairs(float DeltaTime);

	FHUDPackage HUDPackage;

	// hud and crosshairs
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

/************************************************************************/
/* 更换弹夹
/************************************************************************/
public:
	void Reload();
	
	UFUNCTION(Server, Reliable)
	void ServerReload();

	void ReloadEmptyWeapon(); // 自动换单

	void HandleReload(); // 服务器和客户端上相同的处理逻辑

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	void UpdateCarriedAmmo();

	int32 AmountToReload(); // 计算换弹时需要填充多少子弹

	void UpdateAmmoValues(); // 更新墙内子弹和携带的子弹数量（HUD）
	
	void UpdateShotgunAmmoValues(); // 霰弹枪有特意化处理
	void JumpToShotgunEnd(); 
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();  // 只有服务端有权限，由MTReload蒙太奇中的Shell通知触发

	// 当前武器类型的所携带的弹药数量，根据EWeaponType决定
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap;  // 注意Tmap类型无法复制,因为服务器和客户端产生哈希值不一定相同

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500;

	void InitializeCarriedAmmo();

	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	UPROPERTY(EditAnywhere)
	int32 StartingRcoketAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 60;

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 120;

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 0;

/************************************************************************/
/*   Grenade                                                          */
/************************************************************************/
	void ThrowGrenade();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished(); // 动画通知

	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade(); // 动画通知

	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);
	
	void ShowAttachedGrenade(bool bShowGrenade);

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	UPROPERTY(ReplicatedUsing = OnRep_Grenades)
	int32 GrenadeAmount = 4;
	UFUNCTION()
	void OnRep_Grenades();

	FORCEINLINE int32 GetGrenadeAmount() const { return GrenadeAmount; }

	UPROPERTY(EditAnywhere)
	int32 MaxGrenadeAmount = 4;

	void UpdateHUDGrenadeAmount();



	
};
