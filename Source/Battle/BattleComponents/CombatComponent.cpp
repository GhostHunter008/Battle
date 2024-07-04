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
#include "Camera/CameraComponent.h"
#include "Sound/SoundCue.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600;
	AimWalkSpeed = 450;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (BattleCharacter)
	{
		BattleCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (UCameraComponent* FollowCamera = BattleCharacter->GetFollowCamera())
		{
			DefaultFOV = FollowCamera->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}

	// 初始化弹药
	if (BattleCharacter->HasAuthority())
	{
		InitializeCarriedAmmo();
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (BattleCharacter && BattleCharacter->IsLocallyControlled())
	{
		// 准星扩散
		SetHUDCrosshairs(DeltaTime);

		// 为了设置武器旋转
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		// 瞄准视野变换
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo,COND_OwnerOnly); // 只复制到拥有的客户端
	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::EquipWeapon(class AWeapon* InWeapon)
{
	if (BattleCharacter == nullptr || InWeapon == nullptr) return;

	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}

	EquippedWeapon = InWeapon;
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = BattleCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, BattleCharacter->GetMesh());
	}
	EquippedWeapon->SetOwner(BattleCharacter); // Owner引擎已经帮我们做好了同步
	EquippedWeapon->SetupHUDAmmo(); // 服务端通过直接调用设置，客户端通过rep_owner设置弹药


	// 设置携带的弹药
	BattleController = BattleController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->GetController()) : BattleController;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	if (BattleController)
	{
		BattleController->SetHUDCarryAmmo(CarriedAmmo);
	}


	// 由controller接管旋转
	BattleCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	BattleCharacter->bUseControllerRotationYaw = true;

	// 播放声音
	if (EquippedWeapon && EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), EquippedWeapon->EquipSound, EquippedWeapon->GetActorLocation());
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && BattleCharacter)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = BattleCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, BattleCharacter->GetMesh());
		}

		// 由controller接管旋转
		BattleCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		BattleCharacter->bUseControllerRotationYaw = true;
	}

	// 播放声音
	if (EquippedWeapon && EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), EquippedWeapon->EquipSound, EquippedWeapon->GetActorLocation());
	}
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

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2, ViewportSize.Y / 2);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		// 此处对检测的起点位置进行前移，能有效防止检测到自身和检测到背后物体的情况发生
		if (BattleCharacter)
		{
			float DistanceToCharacter = (BattleCharacter->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		GetWorld()->LineTraceSingleByChannel
		(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
		//else
		//{
		//	DrawDebugSphere(GetWorld(), TraceHitResult.ImpactPoint, 12, 12, FColor::Red,false,2);
		//}

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (BattleCharacter == nullptr || BattleCharacter->Controller == nullptr) return;

	BattleController = BattleController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->Controller) : BattleController;
	if (BattleController)
	{
		BattleHUD = BattleHUD == nullptr ? Cast<ABattleHUD>(BattleController->GetHUD()) : BattleHUD;
		if (BattleHUD)
		{
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
			FVector2D WalkSpeedRange(0, BattleCharacter->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0, 1);
			FVector Velocity = BattleCharacter->GetVelocity();
			Velocity.Z = 0;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			// 是否在空中，如跳跃、坠落等
			// 范围 【0，2.25】
			if (BattleCharacter->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25, DeltaTime, 2.25);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			// 是否瞄准，注意此处是正值，最后需要减去
			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 30.f);

			HUDPackage.CrosshairSpread = 0.5 + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor;
			BattleHUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	if (BattleCharacter)
	{
		UCameraComponent* FollowCamera = BattleCharacter->GetFollowCamera();
		FollowCamera->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::FireButtonPress(bool bPressed)
{
	bFireButtonPressed=bPressed;

	if (bFireButtonPressed && EquippedWeapon)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;
		ServerFire(HitTarget);
		if (EquippedWeapon)
		{
			CrosshairShootingFactor = .75f;
		}
		StartFireTimer();
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (BattleCharacter && CombatState == ECombatState::ECS_Unoccupied)
	{
		BattleCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || BattleCharacter == nullptr) return;
	BattleCharacter->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}

	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

bool UCombatComponent::CanFire()
{
	if(EquippedWeapon==nullptr) return false;

	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState==ECombatState::ECS_Unoccupied;
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRcoketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState!= ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (BattleCharacter==nullptr || EquippedWeapon == nullptr) return;

	CombatState=ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	}
}

void UCombatComponent::FinishReloading()
{
	if(BattleCharacter==nullptr) return;

	if (BattleCharacter->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

void UCombatComponent::UpdateAmmoValues()
{
	if (BattleCharacter == nullptr || EquippedWeapon == nullptr) return;

	int32 ReloadInMag = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadInMag;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	// 设置携带弹药的数量
	BattleController = BattleController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->GetController()) : BattleController;
	if (BattleController)
	{
		BattleController->SetHUDCarryAmmo(CarriedAmmo);
	}
	// 设置武器中弹药的数量
	EquippedWeapon->AddAmmo(ReloadInMag);
}

void UCombatComponent::HandleReload()
{
	BattleCharacter->PlayReloadMontage();
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	BattleController = BattleController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->GetController()) : BattleController;
	if (BattleController)
	{
		BattleController->SetHUDCarryAmmo(CarriedAmmo); // 由服务器决定
	}
}
