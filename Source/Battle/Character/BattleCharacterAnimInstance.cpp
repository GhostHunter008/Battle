// Fill out your copyright notice in the Description page of Project Settings.


#include "BattleCharacterAnimInstance.h"
#include "BattleCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Battle/Weapon/Weapon.h"

void UBattleCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BattleCharacter = Cast<ABattleCharacter>(TryGetPawnOwner());

}

void UBattleCharacterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (BattleCharacter==nullptr)
	{
		BattleCharacter= Cast<ABattleCharacter>(TryGetPawnOwner());
	}
	if (BattleCharacter == nullptr) return;

	FVector Velocity=BattleCharacter->GetVelocity();
	Velocity.Z=0;
	Speed=Velocity.Size();

	bIsInAir=BattleCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating=BattleCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;

	bWeaponEquipped=BattleCharacter->IsWeaponEquipped();
	EquippedWeapon=BattleCharacter->GetEquippedWeapon();

	bIsCrouched=BattleCharacter->bIsCrouched;

	bAiming=BattleCharacter->IsAiming();

	// Offest Yaw for Strafing
	FRotator AimRotation = BattleCharacter->GetBaseAimRotation(); // global rotation, 取决于相机的旋转
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BattleCharacter->GetVelocity()); // global rotation,取决于人物的旋转
	FRotator DeltaRot=UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation,AimRotation);
	DeltaRotation=FMath::RInterpTo(DeltaRotation,DeltaRot,DeltaTime,6);
	YawOffset=DeltaRotation.Yaw;
	
	// Lean
	CharacterRotationLastFrame=CharacterRotationCurFrame;
	CharacterRotationCurFrame=BattleCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotationCurFrame,CharacterRotationLastFrame);
	const float Target=Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean,Target,DeltaTime,6);
	Lean=FMath::Clamp(Interp,-90,90);

	// AimOffset
	AO_Yaw=BattleCharacter->GetAOYaw();
	AO_Pitch=BattleCharacter->GetAOPitch();

	//IK
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BattleCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"));
		FVector OutPosition;
		FRotator OutRotation;
		BattleCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"),LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition,OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
	}

	TurningInPlace=BattleCharacter->GetTurningInPlace();
}
