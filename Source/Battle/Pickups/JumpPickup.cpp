// Fill out your copyright notice in the Description page of Project Settings.


#include "JumpPickup.h"
#include "Battle/Character/BattleCharacter.h"
#include "Battle/BattleComponents/BuffComponent.h"

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABattleCharacter* BattleCharacter = Cast<ABattleCharacter>(OtherActor);
	if (BattleCharacter)
	{
		UBuffComponent* Buff = BattleCharacter->GetBuff();
		if (Buff)
		{
			Buff->BuffJump(JumpZVelocityBuff, JumpBuffTime);
		}
	}

	Destroy();
}
