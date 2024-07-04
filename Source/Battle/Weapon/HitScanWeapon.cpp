// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Battle/Character/BattleCharacter.h"
#include "Kismet/GameplayStatics.h"


void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	// HitTarget�Ƿ���˺Ϳͻ���ͬ���ı���
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket && InstigatorController)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		FVector End = Start + (HitTarget - Start) * 1.25f;  // ��Ļ���λ��

		FHitResult FireHit;
		UWorld* World = GetWorld();
		if (World)
		{
			World->LineTraceSingleByChannel(
				FireHit,
				Start,
				End,
				ECollisionChannel::ECC_Visibility
			);
			if (FireHit.bBlockingHit)
			{
				ABattleCharacter* BattleCharacter = Cast<ABattleCharacter>(FireHit.GetActor());
				if (BattleCharacter)
				{
					if (HasAuthority()) // ֻ�з���˲�������˺�
					{
						UGameplayStatics::ApplyDamage(
							BattleCharacter,
							Damage,
							InstigatorController,
							this,
							UDamageType::StaticClass()
						);
					}
				}
				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						World,
						ImpactParticles,
						FireHit.ImpactPoint,
						FireHit.ImpactNormal.Rotation()
					);
				}
			}
		}
	}
}
