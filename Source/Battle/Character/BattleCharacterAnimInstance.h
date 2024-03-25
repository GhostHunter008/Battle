// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Battle/BattleTypes/TurningInPlace.h"
#include "BattleCharacterAnimInstance.generated.h"

/**
 * UAnimInstance不会复制，只会访问自己计算机中的变量
 */
UCLASS()
class BATTLE_API UBattleCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

private:
	UPROPERTY(BlueprintReadOnly,Category=Character,meta =(AllowPrivateAccess="true"))
	class ABattleCharacter* BattleCharacter;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bWeaponEquipped;
	class AWeapon* EquippedWeapon;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsCrouched;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bAiming;


	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float YawOffset;
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float Lean;
	FRotator DeltaRotation;
	FRotator CharacterRotationLastFrame;
	FRotator CharacterRotationCurFrame;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float AO_Yaw;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float AO_Pitch;

	// IK
	// 每个武器都需要新建一个LeftHandSocket的命名插槽
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	FTransform LeftHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	ETurningInPlace TurningInPlace;
};
