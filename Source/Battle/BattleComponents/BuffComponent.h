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
	void Heal(float HealAmount, float HealingTime); // 供外界调用
	void HealRampUp(float DeltaTime); // 逐渐恢复血量

	
	bool bHealing = false;
	float HealingRate = 0; // 每秒钟回复的血量（这样处理因为机器帧率不一样）
	float AmountToHeal = 0.f; // 还剩余需要回复的血量

	/*
	* Speed Buff
	*/
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);  // 速度Buff需要的外调函数
	
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed); // 初始化内部存储的Initial Speed

	FTimerHandle SpeedBuffTimer;
	void ResetSpeeds(); // 恢复默认速度

	// 游戏初始时的速度（Buff结束后需要恢复的速度）
	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	/**
	 * Jump Buff
	 */
	void BuffJump(float BuffJumpVelocity, float BuffTime); // Buff 的外调函数

	void SetInitialJumpVelocity(float Velocity); // 初始化内部存储的InitialJumpVelocity

	FTimerHandle JumpBuffTimer;
	void ResetJump();

	// 游戏初始时的跳跃能力（Buff结束后需要恢复的跳跃能力）
	float InitialJumpVelocity;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);

	/**
	* Shield buff
	*/
	void ReplenishShield(float ShieldAmount, float ReplenishTime); // pickup类外部调用的函数

	void ShieldRampUp(float DeltaTime);

	bool bReplenishingShield = false;

	float ShieldReplenishRate = 0.f;
	float ShieldReplenishAmount = 0.f;

};
