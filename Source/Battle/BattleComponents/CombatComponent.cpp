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
#include "Battle/Weapon/Projectile.h"

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
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo,COND_OwnerOnly); // 只复制到拥有的客户端
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, GrenadeAmount);
}

void UCombatComponent::EquipWeapon(class AWeapon* InWeapon)
{
	if (BattleCharacter == nullptr || InWeapon == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
	{
		EquipSecondaryWeapon(InWeapon);
	}
	else
	{
		EquipPrimaryWeapon(InWeapon);
	}

	// 由controller接管旋转
	BattleCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	BattleCharacter->bUseControllerRotationYaw = true;

	
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;

	DropEquippedWeapon();

	EquippedWeapon = WeaponToEquip;
	ReloadEmptyWeapon();

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetOwner(BattleCharacter); // Owner引擎已经帮我们做好了同步
	EquippedWeapon->SetupHUDAmmo(); // 服务端通过直接调用设置，客户端通过rep_owner设置弹药

	// 设置携带的弹药
	UpdateCarriedAmmo();

	// 播放声音
	PlayEquipWeaponSound(WeaponToEquip);

	EquippedWeapon->EnableCustomDepth(false);
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;

	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToBackpack(WeaponToEquip);

	PlayEquipWeaponSound(WeaponToEquip);

	if (SecondaryWeapon->GetWeaponMesh())
	{
		SecondaryWeapon->GetWeaponMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
		SecondaryWeapon->GetWeaponMesh()->MarkRenderStateDirty();
	}

	if (EquippedWeapon == nullptr) return;
	EquippedWeapon->SetOwner(BattleCharacter);
}

void UCombatComponent::SwapWeapons()
{
	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetupHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);

	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
}

bool UCombatComponent::ShouldSwapWeapons()
{
	return (EquippedWeapon != nullptr && SecondaryWeapon != nullptr);
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && BattleCharacter)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);

		// 由controller接管旋转
		BattleCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		BattleCharacter->bUseControllerRotationYaw = true;
	}

	// 播放声音
	PlayEquipWeaponSound(EquippedWeapon);
	EquippedWeapon->EnableCustomDepth(false);
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{
	if (BattleCharacter && WeaponToEquip && WeaponToEquip->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			WeaponToEquip->EquipSound,
			BattleCharacter->GetActorLocation()
		);
	}
}

void UCombatComponent::SetAiming(bool InbAiming)
{
	if (BattleCharacter == nullptr || EquippedWeapon == nullptr) return;

	bAiming=InbAiming; // 这一行可以不删，在服务器上调用时可以快一步设置
	if (!BattleCharacter->HasAuthority())
	{// Client
		ServerSetAiming(InbAiming); // 因为bAiming是Replicate的变量，因此只有客户端需要RPC
	}
	if (BattleCharacter)
	{
		BattleCharacter->GetCharacterMovement()->MaxWalkSpeed=bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}

	if (BattleCharacter->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		BattleCharacter->ShowSniperScopeWidget(InbAiming);
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
	
	// 霰弹枪特意化处理
	if (BattleCharacter && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		BattleCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
		CombatState = ECombatState::ECS_Unoccupied;
		return;
	}

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

	// 霰弹枪专有逻辑，可以在撞弹过程中进行打断
	if (!EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun) return true;
	
	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState==ECombatState::ECS_Unoccupied;
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRcoketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);
		UpdateCarriedAmmo();
	}
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

void UCombatComponent::ThrowGrenade()
{
	if (GrenadeAmount == 0) return;

	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (BattleCharacter)
	{
		BattleCharacter->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}
	if (BattleCharacter && !BattleCharacter->HasAuthority())
	{
		ServerThrowGrenade();
	}
	// 服务器更新手雷数量
	if (BattleCharacter && BattleCharacter->HasAuthority())
	{
		GrenadeAmount = FMath::Clamp(GrenadeAmount - 1, 0, MaxGrenadeAmount);
		UpdateHUDGrenadeAmount();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (GrenadeAmount == 0) return;
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (BattleCharacter)
	{
		BattleCharacter->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}
	GrenadeAmount = FMath::Clamp(GrenadeAmount - 1, 0, MaxGrenadeAmount);
	UpdateHUDGrenadeAmount(); // 可要可不要，因为HUD只存在于local
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (BattleCharacter == nullptr || BattleCharacter->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const USkeletalMeshSocket* HandSocket = BattleCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, BattleCharacter->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (BattleCharacter == nullptr || BattleCharacter->GetMesh() == nullptr || ActorToAttach == nullptr || EquippedWeapon == nullptr) return;
	// 对手枪和冲锋枪进行一定的特意化处理
	// 不处理位置差的过多
	// 事实上，可以根据武器类型创建不同的lefthandsocket
	bool bUsePistolSocket =
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol ||
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun;
	FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket"); 
	const USkeletalMeshSocket* HandSocket = BattleCharacter->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, BattleCharacter->GetMesh());
	}
}

void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach)
{
	if (BattleCharacter == nullptr || BattleCharacter->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const USkeletalMeshSocket* BackpackSocket = BattleCharacter->GetMesh()->GetSocketByName(FName("BackpackSocket"));
	if (BackpackSocket)
	{
		BackpackSocket->AttachActor(ActorToAttach, BattleCharacter->GetMesh());
	}
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);

	if (BattleCharacter && BattleCharacter->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	const FVector StartingLocation = BattleCharacter->GetAttachedGrenade()->GetComponentLocation();
	FVector ToTarget = Target - StartingLocation;
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = BattleCharacter;
	SpawnParams.Instigator = BattleCharacter;
	UWorld* World = GetWorld();
	if (World)
	{
		World->SpawnActor<AProjectile>(
			GrenadeClass,
			StartingLocation,
			ToTarget.Rotation(),
			SpawnParams
		);
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (BattleCharacter && BattleCharacter->GetAttachedGrenade())
	{
		BattleCharacter->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenadeAmount();
}

void UCombatComponent::UpdateHUDGrenadeAmount()
{
	BattleController = BattleController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->Controller) : BattleController;
	if (BattleController)
	{
		BattleController->SetHUDGrenades(GrenadeAmount);
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && BattleCharacter)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToBackpack(SecondaryWeapon);
		PlayEquipWeaponSound(EquippedWeapon);
		if (SecondaryWeapon->GetWeaponMesh())
		{
			SecondaryWeapon->GetWeaponMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
			SecondaryWeapon->GetWeaponMesh()->MarkRenderStateDirty();
		}
	}
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull())
	{
		ServerReload();
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
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
	{
		HandleReload();
		break;
	}
	case ECombatState::ECS_ThrowingGrenade:
	{
		if (BattleCharacter && !BattleCharacter->IsLocallyControlled())
		{
			BattleCharacter->PlayThrowGrenadeMontage();
			AttachActorToLeftHand(EquippedWeapon);
			ShowAttachedGrenade(true);
		}
		break;
	}
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

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	BattleController = BattleController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->Controller) : BattleController;
	if (BattleController)
	{
		BattleController->SetHUDCarryAmmo(CarriedAmmo);
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

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (BattleCharacter == nullptr || EquippedWeapon == nullptr) return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	// 设置携带弹药的数量（更新UI）
	BattleController = BattleController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->Controller) : BattleController;
	if (BattleController)
	{
		BattleController->SetHUDCarryAmmo(CarriedAmmo);
	}
	// 设置武器中弹药的数量（更新UI）
	EquippedWeapon->AddAmmo(1);


	bCanFire = true; // 无需完整装填，可以继续开枪
	if (EquippedWeapon->IsFull() || CarriedAmmo == 0)
	{
		JumpToShotgunEnd(); // 服务端跳到End动画，还需要在Rep中设置客户端的动画
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	// Jump to ShotgunEnd section
	UAnimInstance* AnimInstance = BattleCharacter->GetMesh()->GetAnimInstance();
	if (AnimInstance && BattleCharacter->GetReloadMontage())
	{
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

void UCombatComponent::ShotgunShellReload()
{
	if (BattleCharacter && BattleCharacter->HasAuthority()) // 仅服务端有权限
	{
		UpdateShotgunAmmoValues();
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	BattleController = BattleController == nullptr ? Cast<ABattlePlayerController>(BattleCharacter->GetController()) : BattleController;
	if (BattleController)
	{
		BattleController->SetHUDCarryAmmo(CarriedAmmo); // 由服务器决定
	}

	// 霰弹枪的逻辑，保证客户端动画正确
	bool bJumpToShotgunEnd =
		CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon != nullptr &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		CarriedAmmo == 0;
	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::HandleReload()
{
	BattleCharacter->PlayReloadMontage();
}
