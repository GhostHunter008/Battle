// Fill out your copyright notice in the Description page of Project Settings.


#include "BattleCharacterAnimInstance.h"
#include "BattleCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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

}
