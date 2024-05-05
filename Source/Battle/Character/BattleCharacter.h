#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Battle/BattleTypes/TurningInPlace.h"
#include "Battle/Interfaces/InteractWithCrosshairInterface.h"
#include "Components/TimelineComponent.h"
#include "Battle/BattleTypes/CombatStates.h"
#include "BattleCharacter.generated.h"



UCLASS()
class BATTLE_API ABattleCharacter : public ACharacter,public IInteractWithCrosshairInterface
{
	GENERATED_BODY()

public:
	ABattleCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; // 注册需要复制的变量
	virtual void PostInitializeComponents() override; // 在组件初始化完成过后自动调用

	virtual void Destroyed() override;


	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }

	// Input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	virtual void Jump() override; // 重写，让跳跃可以解除蹲伏
	void EquipButtonPress(const FInputActionValue& Value);
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed(); // RPC，实现加_Implement后缀
	void CrouchButtonPress(const FInputActionValue& Value);
	void AimButtonPress(const FInputActionValue& Value);
	void AimButtonRelease(const FInputActionValue& Value);
	void FireButtonPress(const FInputActionValue& Value);
	void FireButtonRelease(const FInputActionValue& Value);
	void ReloadButtonPress(const FInputActionValue& Value);

private:
	UPROPERTY(VisibleAnywhere,Category=Camera)
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true")) // AllowPrivateAccess将私有变量暴露给蓝图
	class UInputMappingContext* DefaultMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* EquipAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* CrouchAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* AimAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* FireAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget; // 用于在人物上方显示信息

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon) // 仅在发生变化时同步，只同步到客户端
	class AWeapon* OverlappingWeapon;
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* CombatComponent;
	

	class ABattlePlayerController* BattlePlayerController;

	float AO_Yaw;
	float Interp_AO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	bool bRotateRootBone;

	float TurnThreshold=0.5f; // 超过这个阈值simulate播放转身动画
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere,Category=Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* HitReactMontage;


	void HideCharacterIfCameraClose();
	UPROPERTY(EditAnywhere)
	float CameraThreshold=200.f;





public:	
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();
	void RotateInPlace(float DeltaTime);
	FORCEINLINE float GetAOYaw(){return AO_Yaw;}
	FORCEINLINE float GetAOPitch() { return AO_Pitch;}
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace()const {return TurningInPlace;}
	void PlayFireMontage(bool bAiming);
	void PlayHitReactMontage();
	void PlayReloadMontage();
	//UFUNCTION(NetMulticast, Unreliable) // 通过health的rep函数走，不再用RPC
	//void MulticastHit();
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const{return FollowCamera;}
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	virtual void OnRep_ReplicatedMovement() override; // 每当同步物体运动时会被调用
	float CalculateSpeed();
	FORCEINLINE UCombatComponent* GetCombat() const { return CombatComponent; }


	/*
	*  Player Health
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PlayerStats, meta = (AllowPrivateAccess = "true"))
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, EditAnywhere, BlueprintReadOnly, Category = PlayerStats, meta = (AllowPrivateAccess = "true"))
	float Health = 100.f;

	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	void UpdateHUDHealth();

	void Elim(); // 仅在服务器端执行
	UFUNCTION(NetMulticast,Reliable)
	void MulticastElim(); // 淘汰时处理的逻辑

	UPROPERTY(EditAnywhere,Category=Combat)
	UAnimMontage* ElimMontage;

	void PlayElimMontage();

	bool bElimmed=false;
	FORCEINLINE bool IsElimmed() const{return bElimmed;}

	FTimerHandle ElimTimer;
	UPROPERTY(EditDefaultsOnly)
	float ElimDelayTime=3;
	void ElimTimerFinished();

	/* 
	* Elim Dissolve effect 
	*/

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimelineComp;

	FOnTimelineFloat DissolveTrack; // 轨道

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve; // 蓝图中配置
	
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	void StartDissolve();
	
	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	/*
	* Elim Bot
	*/
	UPROPERTY(EditAnywhere, Category = Elim)
	UParticleSystem* ElimBotEffect; // 蓝图中配置

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotEffectComp; // 存储该特效,用于销毁

	UPROPERTY(EditAnywhere, Category = Elim)
	class USoundCue* ElimBotSound;

	// poll for any related class to init our hud
	void PollInit();

	class ABattlePlayerState* BattlePlayerState;


	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

	/*
	* Reload
	*/
	ECombatState GetCombatState() const;







};
