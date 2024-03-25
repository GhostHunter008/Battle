#include "CombatComponent.h"
#include "Battle/Weapon/Weapon.h"
#include "Battle/Character/BattleCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Battle/PlayerController/BattlePlayerController.h"
#include "Battle//HUD/BattleHUD.h"



UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600;
	AimWalkSpeed = 450;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (BattleCharacter)
	{
		BattleCharacter->GetCharacterMovement()->MaxWalkSpeed=BaseWalkSpeed;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SetHUDCrosshairs(DeltaTime);
}

void UCombatComponent::SetAiming(bool InbAiming)
{
	bAiming=InbAiming; // 这一行可以不删，在服务器上调用时可以快一步设置
	if (!BattleCharacter->HasAuthority())
	{// Client
		ServerSetAiming(InbAiming); // 因为bAiming是Replicate的变量，因此只有客户端需要RPC
	}
	if (BattleCharacter)
	{
		BattleCharacter->GetCharacterMovement()->MaxWalkSpeed=bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool InbAiming)
{
	bAiming = InbAiming;
	if (BattleCharacter)
	{
		BattleCharacter->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && BattleCharacter)
	{
		// 由controller接管旋转
		BattleCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		BattleCharacter->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::FireButtonPress(bool bPressed)
{
	FireButtonPressed=bPressed;

	if (FireButtonPressed)
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);

		ServerFire(HitResult.ImpactPoint);
		
	}
	
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (BattleCharacter)
	{
		BattleCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X/2,ViewportSize.Y/2);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this,0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		FVector End= Start + CrosshairWorldDirection * TRACE_LENGTH;
		GetWorld()->LineTraceSingleByChannel
		(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint=End;
		}
		//DrawDebugSphere(GetWorld(),TraceHitResult.ImpactPoint,12,12,FColor::Red);
	}

}



void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if(BattleCharacter==nullptr || BattleCharacter->Controller==nullptr) return;

	BattleController = BattleController==nullptr ? Cast<ABattlePlayerController>(BattleCharacter->Controller) : BattleController;
	if (BattleController)
	{
		BattleHUD = BattleHUD==nullptr ? Cast<ABattleHUD>(BattleController->GetHUD()) : BattleHUD;
		if (BattleHUD)
		{
			FHUDPackage HUDPackage;
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}

			// Calculate crosshair spread
			// 影响因素
			// 移动速度 【0，600】-【0，1】
			FVector2D WalkSpeedRange(0,BattleCharacter->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0,1);
			FVector Velocity=BattleCharacter->GetVelocity();
			Velocity.Z=0;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange,VelocityMultiplierRange,Velocity.Size());

			// 是否在空中，如跳跃、坠落等
			if (BattleCharacter->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor=FMath::FInterpTo(CrosshairInAirFactor,2.25,DeltaTime,2.25);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0, DeltaTime, 30);
			}



			HUDPackage.CrosshairSpread=CrosshairVelocityFactor + CrosshairInAirFactor;
			BattleHUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::EquipWeapon(class AWeapon* InWeapon)
{
	if(BattleCharacter==nullptr || InWeapon==nullptr) return;

	EquippedWeapon=InWeapon;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = BattleCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon,BattleCharacter->GetMesh());
	}
	EquippedWeapon->SetOwner(BattleCharacter); // Owner引擎已经帮我们做好了同步

	// 由controller接管旋转
	BattleCharacter->GetCharacterMovement()->bOrientRotationToMovement=false;
	BattleCharacter->bUseControllerRotationYaw=true;

}

