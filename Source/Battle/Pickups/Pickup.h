// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

UCLASS()
class BATTLE_API APickup : public AActor
{
	GENERATED_BODY()
	
public:
	APickup();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor,UPrimitiveComponent* OtherComp,int32 OtherBodyIndex,bool bFromSweep,const FHitResult& SweepResult);

	UPROPERTY(EditAnywhere)
	float BaseTurnRate = 45.f;

private:

	UPROPERTY(EditAnywhere)
	class USphereComponent* OverlapSphere;

	UPROPERTY(EditAnywhere)
	class USoundCue* PickupSound;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* PickupMesh;

	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* PickupEffectComponent; // 用来显示场景中固定的“Mesh”

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* PickupEffect; // 配置资源，用来表示使用的效果

	/*
	*  延迟绑定Overlap
	*  过早绑定会出现bug:玩家刚碰到物体，物体就被销毁
	*  导致pickup spawnpoint中StartSpawnPickupTimer函数尚未绑定，导致spawnpoint无法再次生成物体
	*/
	FTimerHandle BindOverlapTimer;
	float BindOverlapTime = 0.25f;
	UFUNCTION()
	void BindOverlapTimerFinished();
	
};
