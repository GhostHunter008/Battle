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
	TArray<TSubclassOf<class APickup>> PickupClasses; // ������ɵ�Pickup

	UPROPERTY()
	APickup* SpawnedPickup; // ���ɵ�Pickup�������������¼�ʱ���¿�ʼ����

	
	void SpawnPickupTimerFinished(); // ���ʱ�����������SpawnPickup 

	void SpawnPickup();


private:
	FTimerHandle SpawnPickupTimer;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;

};
