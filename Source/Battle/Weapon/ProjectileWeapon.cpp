// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	// 只有服务器能射击
	if(!HasAuthority()) return;


	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FRotator TargetRotation = (HitTarget-SocketTransform.GetLocation()).Rotation();
		if (ProjectileClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner=GetOwner(); // 在玩家拾取时设置了owner
			SpawnParams.Instigator=Cast<APawn>(GetOwner());

			UWorld* World=GetWorld();
			if (World)
			{
				World->SpawnActor<AProjectile>
				(	
					ProjectileClass,
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParams
				);
			}
		}	
	}
}
