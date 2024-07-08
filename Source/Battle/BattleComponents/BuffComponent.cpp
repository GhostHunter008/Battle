#include "BuffComponent.h"
#include "Battle/Character/BattleCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}


void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();

	
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime); // 逐步回复生命值
	ShieldRampUp(DeltaTime); // 逐步回复蓝盾
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing || BattleCharacter == nullptr || BattleCharacter->IsElimmed()) return;

	const float HealThisFrame = HealingRate * DeltaTime; // 取消不同机器帧率不同的干扰
	BattleCharacter->SetHealth(FMath::Clamp(BattleCharacter->GetHealth() + HealThisFrame, 0.f, BattleCharacter->GetMaxHealth()));
	BattleCharacter->UpdateHUDHealth();
	AmountToHeal -= HealThisFrame;

	if (AmountToHeal <= 0.f || BattleCharacter->GetHealth() >= BattleCharacter->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0.f;
	}
}

void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime)
{
	if (BattleCharacter == nullptr) return;

	BattleCharacter->GetWorldTimerManager().SetTimer(
		SpeedBuffTimer,
		this,
		&UBuffComponent::ResetSpeeds,
		BuffTime
	);

	// 修改服务端的角色速度
	if (BattleCharacter->GetCharacterMovement())
	{
		BattleCharacter->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
		BattleCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
	}
	// 修改客户端的角色速度
	MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
}

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::ResetSpeeds()
{
	if (BattleCharacter == nullptr || BattleCharacter->GetCharacterMovement() == nullptr) return;

	BattleCharacter->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	BattleCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	BattleCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	BattleCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void UBuffComponent::BuffJump(float BuffJumpVelocity, float BuffTime)
{
	if (BattleCharacter == nullptr) return;

	BattleCharacter->GetWorldTimerManager().SetTimer(
		JumpBuffTimer,
		this,
		&UBuffComponent::ResetJump,
		BuffTime
	);

	if (BattleCharacter->GetCharacterMovement())
	{
		BattleCharacter->GetCharacterMovement()->JumpZVelocity = BuffJumpVelocity;
	}
	MulticastJumpBuff(BuffJumpVelocity);
}

void UBuffComponent::SetInitialJumpVelocity(float Velocity)
{
	InitialJumpVelocity = Velocity;
}

void UBuffComponent::ResetJump()
{
	if (BattleCharacter->GetCharacterMovement())
	{
		BattleCharacter->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;
	}
	MulticastJumpBuff(InitialJumpVelocity);
}

void UBuffComponent::MulticastJumpBuff_Implementation(float JumpVelocity)
{
	if (BattleCharacter && BattleCharacter->GetCharacterMovement())
	{
		BattleCharacter->GetCharacterMovement()->JumpZVelocity = JumpVelocity;
	}
}

void UBuffComponent::ReplenishShield(float ShieldAmount, float ReplenishTime)
{
	bReplenishingShield = true;
	ShieldReplenishRate = ShieldAmount / ReplenishTime;
	ShieldReplenishAmount += ShieldAmount;
}

void UBuffComponent::ShieldRampUp(float DeltaTime)
{
	if (!bReplenishingShield || BattleCharacter == nullptr || BattleCharacter->IsElimmed()) return;

	const float ReplenishThisFrame = ShieldReplenishRate * DeltaTime;
	BattleCharacter->SetShield(FMath::Clamp(BattleCharacter->GetShield() + ReplenishThisFrame, 0.f, BattleCharacter->GetMaxShield()));
	BattleCharacter->UpdateHUDShield();
	ShieldReplenishAmount -= ReplenishThisFrame;

	if (ShieldReplenishAmount <= 0.f || BattleCharacter->GetShield() >= BattleCharacter->GetMaxShield())
	{
		bReplenishingShield = false;
		ShieldReplenishAmount = 0.f;
	}
}

