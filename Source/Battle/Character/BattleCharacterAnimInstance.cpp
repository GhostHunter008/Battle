// Fill out your copyright notice in the Description page of Project Settings.


#include "BattleCharacterAnimInstance.h"
#include "BattleCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Battle/Weapon/Weapon.h"
#include "Battle/BattleTypes/CombatStates.h"

void UBattleCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BattleCharacter = Cast<ABattleCharacter>(TryGetPawnOwner());

}

void UBattleCharacterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	// 确保BattleCharacter有效
	if (BattleCharacter==nullptr)
	{
		BattleCharacter= Cast<ABattleCharacter>(TryGetPawnOwner());
	}
	if (BattleCharacter == nullptr) return;

	// 计算Speed，不考虑Z轴速度
	FVector Velocity=BattleCharacter->GetVelocity();
	Velocity.Z=0;
	Speed=Velocity.Size();

	bIsInAir=BattleCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating=BattleCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;

	bWeaponEquipped=BattleCharacter->IsWeaponEquipped();
	EquippedWeapon=BattleCharacter->GetEquippedWeapon();

	bIsCrouched=BattleCharacter->bIsCrouched;

	bAiming=BattleCharacter->IsAiming();

	// Offest Yaw和Lean计算都建立在已经复制过的值的基础之上
	// 每个机器跑自己的实例，因此也无需复制但是能相同
	// Offest Yaw for Strafing
	FRotator AimRotation = BattleCharacter->GetBaseAimRotation(); // global rotation, 取决于相机的旋转，世界X轴为0，Y轴为90
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BattleCharacter->GetVelocity()); // global rotation,速度是一个向量，取决于人物运动的朝向
	FRotator DeltaRot=UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation,AimRotation);
	DeltaRotation=FMath::RInterpTo(DeltaRotation,DeltaRot,DeltaTime,6); // 插值时会走最短路径
	YawOffset=DeltaRotation.Yaw;
	
	// Lean : 用鼠标旋转的速度衡量
	CharacterRotationLastFrame=CharacterRotationCurFrame;
	CharacterRotationCurFrame=BattleCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotationCurFrame,CharacterRotationLastFrame);
	const float Target=Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean,Target,DeltaTime,6);
	Lean=FMath::Clamp(Interp,-90,90);

	// AimOffset
	AO_Yaw=BattleCharacter->GetAOYaw();
	AO_Pitch=BattleCharacter->GetAOPitch();

	// HandsTransform : IK 和 武器旋转修正
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BattleCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket")); // 获取的是LeftHandSocket世界空间位置
		FVector OutPosition;
		FRotator OutRotation;
		// 将LeftHandSocket世界空间位置以右手的位置为参考，转到骨骼空间下的坐标
		BattleCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"),LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition,OutRotation); 
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (BattleCharacter->IsLocallyControlled())
		{
			bLocallyControlled=true;
			//FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"));
			FTransform RightHandTransform = BattleCharacter->GetMesh()->GetSocketTransform(FName("hand_r"));
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BattleCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation,LookAtRotation,DeltaTime,30);
			
			//// Debug
			//// 从枪口发出的射线
			//FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
			//FVector MuzzleX = FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X);
			//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
			//// 从枪口到目标点（准星决定）的射线
			//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), BattleCharacter->GetHitTarget(), FColor::Orange);
		}
	}

	// TurningInPlace
	TurningInPlace=BattleCharacter->GetTurningInPlace();

	bRotateRootBone = BattleCharacter->ShouldRotateRootBone();

	bElimmed=BattleCharacter->IsElimmed();

	bUseFABRIK = BattleCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
	bUseAimOffset = BattleCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BattleCharacter->GetDisableGameplay();
	bTransformRightHand = BattleCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BattleCharacter->GetDisableGameplay();
}
