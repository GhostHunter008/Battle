#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM()
enum class EWeaponState:uint8
{
	EWS_Initial UMETA(DisplayName="Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped State"),
	EWS_Dropped UMETA(DisplayName = "Dropped State"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX") // 可以帮助我们快速知道枚举的数量
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
	FORCEINLINE class USphereComponent* GetAreaSphere() const{return AreaSphere;}
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const{return WeaponMesh;}

	virtual void Fire(const FVector& HitTarget);

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
};
