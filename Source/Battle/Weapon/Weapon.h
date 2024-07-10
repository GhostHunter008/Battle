#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Battle/Weapon/WeaponTypes.h"
#include "Weapon.generated.h"

UENUM()
enum class EWeaponState:uint8
{
	EWS_Initial UMETA(DisplayName="Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped State"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped State"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX") // 可以帮助我们快速知道枚举的数量
};

// 原来是服务端和客户端各自算各自的，但是以服务端为准
// 客户端把本地计算的结果传给服务器，进行命中判定
// 这个枚举体用来表示传给服务端的数据
UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),

	EFT_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class BATTLE_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ShowPickupWidget(bool bShowWidget);

	void SetWeaponState(EWeaponState InWeaponState);
	virtual void OnWeaponStateSet();

	virtual void OnEquipped();
	virtual void OnEquippedSecondary();
	virtual void OnDropped();

	FORCEINLINE class USphereComponent* GetAreaSphere() const{return AreaSphere;}
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const{return WeaponMesh;}

	virtual void Fire(const FVector& HitTarget);

	UPROPERTY(EditAnywhere)
	EFireType FireType;

protected:
	virtual void BeginPlay() override;

	// 绑定到AreaSphere的Overlap函数
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);


private:
	UPROPERTY(VisibleAnywhere, Category = "WeaponProperties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "WeaponProperties")
	class USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = "WeaponProperties", ReplicatedUsing = OnRep_WeaponState)
	EWeaponState WeaponState;
	UFUNCTION()
	void OnRep_WeaponState(); // ShowPickupWidget在服务端执行后并不会复制，但状态会复制，可以通过RepNotify来实现

	UPROPERTY(VisibleAnywhere, Category = "WeaponProperties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "WeaponProperties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABulletShell> BulletShellClass;


	class ABattleCharacter* BattleOwnerCharacter=nullptr;
	class ABattlePlayerController* BattleOwnerPlayerController=nullptr;

public:
	/*
	* Texture for the weapon crosshairs
	*/
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	class UTexture2D* CrosshairsCenter;
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	class UTexture2D* CrosshairsLeft;
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	class UTexture2D* CrosshairsRight;
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	class UTexture2D* CrosshairsTop;
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	class UTexture2D* CrosshairsBottom;

public:
	/* Zoom FOV while aiming */
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed=20.f;

	FORCEINLINE float GetZoomedFOV() const{return ZoomedFOV;}
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }

	/**
	* Automatic fire
	*/
	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = .15f;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	/*
	*  Dropped
	*/
	void Dropped();


	/*
	* 弹药
	*/
	UPROPERTY(EditAnywhere)
	int32 MagCapacity; // 枪中的容量
	 
	//UPROPERTY(EditAnywhere,ReplicatedUsing=OnRep_Ammo)
	//int32 Ammo; // 枪中的子弹
	//UFUNCTION()
	//void OnRep_Ammo();

	UPROPERTY(EditAnywhere)
	int32 Ammo;  // 客户端预测

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	// The number of unprocessed server requests for Ammo.
	// Incremented in SpendRound, decremented in ClientUpdateAmmo.
	int32 Sequence = 0;

	void SpendRound(); // 减少弹药的操作应该只在服务器调用

	void SetupHUDAmmo();

	void AddAmmo(int32 AddAmmo);

	virtual void OnRep_Owner() override;

	bool IsEmpty();
	bool IsFull();

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
	FORCEINLINE EWeaponType GetWeaponType() { return WeaponType; }

	FORCEINLINE int32 GetAmmo(){return Ammo;}
	FORCEINLINE int32 GetMagCapacity() { return MagCapacity; }

	/**
	* Enable or disable custom depth
	*/
	void EnableCustomDepth(bool bEnable);

	// 如果是玩家出生时的自带武器，那么在玩家死亡时该武器会被销毁
	// 否则，该武器会掉落
	bool bDestroyWeapon = false;

	/**
	* Trace end with scatter
	*/
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f; // 以该距离进行画球进行散射
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;  // 散射球的球体半径
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	FVector TraceEndWithScatter(const FVector& HitTarget); // 以枪口位置为起点

};
