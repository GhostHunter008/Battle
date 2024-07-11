// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BATTLE_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

protected:
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);

	//UPROPERTY(EditAnywhere)  // 移动到weapon类中
	//float Damage = 20.f; 

	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* HitSound;

private:

	UPROPERTY(EditAnywhere)
	class UParticleSystem* BeamParticles;

	// 有些资产没有武器动画，则自己配置
	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;
	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;
	//UPROPERTY(EditAnywhere)
	//USoundCue* HitSound;
};
