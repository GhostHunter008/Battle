// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

UCLASS()
class BATTLE_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:
	APickupSpawnPoint();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class APickup>> PickupClasses; // 随机生成的Pickup

	UPROPERTY()
	APickup* SpawnedPickup; // 生成的Pickup，并绑定其销毁事件时重新开始生成

	
	void SpawnPickupTimerFinished(); // 随机时间结束，调用SpawnPickup 

	void SpawnPickup();


private:
	FTimerHandle SpawnPickupTimer;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;

};
