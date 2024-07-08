// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "Battle/Character/BattleCharacter.h"
#include "Battle/BattleComponents/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABattleCharacter* BattleCharacter = Cast<ABattleCharacter>(OtherActor);
	if (BattleCharacter)
	{
		UCombatComponent* Combat = BattleCharacter->GetCombat();
		if (Combat)
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}
	Destroy();
}

