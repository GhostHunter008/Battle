#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BATTLE_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuffComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	friend class ABattleCharacter;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY()
	class ABattleCharacter* BattleCharacter;

	/*
	* Health Buff
	*/
	void Heal(float HealAmount, float HealingTime); // ��������
	void HealRampUp(float DeltaTime); // �𽥻ָ�Ѫ��

	
	bool bHealing = false;
	float HealingRate = 0; // ÿ���ӻظ���Ѫ��������������Ϊ����֡�ʲ�һ����
	float AmountToHeal = 0.f; // ��ʣ����Ҫ�ظ���Ѫ��

	/*
	* Speed Buff
	*/
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);  // �ٶ�Buff��Ҫ���������
	
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed); // ��ʼ���ڲ��洢��Initial Speed

	FTimerHandle SpeedBuffTimer;
	void ResetSpeeds(); // �ָ�Ĭ���ٶ�

	// ��Ϸ��ʼʱ���ٶȣ�Buff��������Ҫ�ָ����ٶȣ�
	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	/**
	 * Jump Buff
	 */
	void BuffJump(float BuffJumpVelocity, float BuffTime); // Buff ���������

	void SetInitialJumpVelocity(float Velocity); // ��ʼ���ڲ��洢��InitialJumpVelocity

	FTimerHandle JumpBuffTimer;
	void ResetJump();

	// ��Ϸ��ʼʱ����Ծ������Buff��������Ҫ�ָ�����Ծ������
	float InitialJumpVelocity;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);

	/**
	* Shield buff
	*/
	void ReplenishShield(float ShieldAmount, float ReplenishTime); // pickup���ⲿ���õĺ���

	void ShieldRampUp(float DeltaTime);

	bool bReplenishingShield = false;

	float ShieldReplenishRate = 0.f;
	float ShieldReplenishAmount = 0.f;

};
