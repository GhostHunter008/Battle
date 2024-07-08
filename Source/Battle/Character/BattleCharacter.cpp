#include "BattleCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Battle/Weapon/Weapon.h"
#include "Battle/BattleComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Battle/Battle.h"
#include "Battle/PlayerController/BattlePlayerController.h"
#include "Battle/GameMode/BattleGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Battle/PlayerState/BattlePlayerState.h"
#include "Battle/Weapon/WeaponTypes.h"
#include "Battle/BattleComponents/BuffComponent.h"


ABattleCharacter::ABattleCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SpawnCollisionHandlingMethod= ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	NetUpdateFrequency=66; // 网络平稳时replicate的频率
	MinNetUpdateFrequency=33; // 网络波动时replicate的频率

	CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength=600;
	CameraBoom->SetRelativeLocation(FVector(0,0,88));
	CameraBoom->bUsePawnControlRotation=true;

	FollowCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom,USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation=false;

	bUseControllerRotationYaw=false;
	GetCharacterMovement()->bOrientRotationToMovement=true;

	OverheadWidget=CreateDefaultSubobject<UWidgetComponent>("OverheadWidget");
	OverheadWidget->SetupAttachment(GetRootComponent());

	CombatComponent=CreateDefaultSubobject<UCombatComponent>("CombatComponent");
	CombatComponent->SetIsReplicated(true); // Component无需在GetLifetimeReplicatedProps中注册

	GetCharacterMovement()->NavAgentProps.bCanCrouch=true; //赋予蹲的能力。蓝图中也可以勾选，这里是启用默认值
	GetCharacterMovement()->RotationRate.Yaw=850; // 转身的速度
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	TurningInPlace=ETurningInPlace::ETIP_NotTurning;

	DissolveTimelineComp = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BuffComponent = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	BuffComponent->SetIsReplicated(true);
}

void ABattleCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ABattleCharacter,OverlappingWeapon);
	DOREPLIFETIME_CONDITION(ABattleCharacter, OverlappingWeapon,COND_OwnerOnly); // 只同步到拥有的客户端（确定了同步给谁）
	DOREPLIFETIME(ABattleCharacter,Health);
	DOREPLIFETIME(ABattleCharacter,bDisableGameplay);
	DOREPLIFETIME(ABattleCharacter, Shield);
}

void ABattleCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (CombatComponent)
	{
		CombatComponent->BattleCharacter=this;
	}
	if (BuffComponent)
	{
		BuffComponent->BattleCharacter = this;
		BuffComponent->SetInitialSpeeds(GetCharacterMovement()->MaxWalkSpeed,GetCharacterMovement()->MaxWalkSpeedCrouched);
		BuffComponent->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}
}

void ABattleCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotEffectComp)
	{
		ElimBotEffectComp->DestroyComponent();
	}
}

void ABattleCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	UpdateHUDHealth();
	UpdateHUDShield();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABattleCharacter::ReceiveDamage);
	}

	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

void ABattleCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);

	HideCharacterIfCameraClose();

	PollInit(); // 轮询 因为第一帧中playerstate无效
}

void ABattleCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ABattleCharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABattleCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABattleCharacter::Look);
		EnhancedInputComponent->BindAction(EquipAction,ETriggerEvent::Started,this, &ABattleCharacter::EquipButtonPress);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABattleCharacter::CrouchButtonPress);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ABattleCharacter::AimButtonPress);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ABattleCharacter::AimButtonRelease);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ABattleCharacter::FireButtonPress);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ABattleCharacter::FireButtonRelease);
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ABattleCharacter::ReloadButtonPress);
		EnhancedInputComponent->BindAction(ThrowGrenadeAction, ETriggerEvent::Started, this, &ABattleCharacter::GrenadeButtonPressed);
	}
}

void ABattleCharacter::Move(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;

	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}

	// GEngine->AddOnScreenDebugMessage(-1,2,FColor::Red,TEXT("MOVE"));
}

void ABattleCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ABattleCharacter::Jump()
{
	if (bDisableGameplay) return;

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABattleCharacter::EquipButtonPress(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;

	bool bEquipped = Value.Get<bool>();

	if (CombatComponent) 
	{
		if(HasAuthority()) // only server
		{
			CombatComponent->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed(); // send RPC
		}
	}
}

void ABattleCharacter::ServerEquipButtonPressed_Implementation()
{
	if (CombatComponent)
	{
		CombatComponent->EquipWeapon(OverlappingWeapon);
	}
}

void ABattleCharacter::CrouchButtonPress(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;
	if (bIsCrouched) //虚幻内置已经处理好，自动同步过
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
	
}

void ABattleCharacter::AimButtonPress(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;
	if (CombatComponent)
	{
		CombatComponent->SetAiming(true);
	}
}

void ABattleCharacter::AimButtonRelease(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;
	if (CombatComponent)
	{
		CombatComponent->SetAiming(false);
	}
}

void ABattleCharacter::FireButtonPress(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;
	if (CombatComponent)
	{
		CombatComponent->FireButtonPress(true);
	}
}

void ABattleCharacter::FireButtonRelease(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;
	if (CombatComponent)
	{
		CombatComponent->FireButtonPress(false);
	}
}

void ABattleCharacter::ReloadButtonPress(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;
	if (CombatComponent)
	{
		CombatComponent->Reload();
	}
}	

void ABattleCharacter::GrenadeButtonPressed(const FInputActionValue& Value)
{
	if (CombatComponent)
	{
		CombatComponent->ThrowGrenade();
	}
}

void ABattleCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon) // 确定了接收到Rep的行为
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{	
		LastWeapon->ShowPickupWidget(false);
	}	
}


// 服务端单独处理：实质上这个函数也只有服务端回调
void ABattleCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;
	if (IsLocallyControlled()) // 服务器端的逻辑单独处理
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABattleCharacter::IsWeaponEquipped()
{
	return (CombatComponent && CombatComponent->EquippedWeapon);
}

bool ABattleCharacter::IsAiming()
{
	return (CombatComponent && CombatComponent->bAiming);
}

void ABattleCharacter::AimOffset(float DeltaTime)
{
	if (CombatComponent==nullptr || CombatComponent->EquippedWeapon==nullptr) return;
	
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed <= 0.1 && !bIsInAir) // standing still
	{
		bRotateRootBone=true;

		FRotator CurrentAimRotation= FRotator(0, GetBaseAimRotation().Yaw, 0);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw=DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			Interp_AO_Yaw=AO_Yaw; // Interp_AO_Yaw在转身时逐步插值为0
		}
		bUseControllerRotationYaw=true;

		TurnInPlace(DeltaTime);

		/*
		if (HasAuthority() && !IsLocallyControlled())
		{
			UE_LOG(LogTemp, Warning, TEXT("standing still"));
			UE_LOG(LogTemp, Warning, TEXT("AO_Yaw: %f"), AO_Yaw);
			UE_LOG(LogTemp, Warning, TEXT("CurrentAimRotation_Yaw: %f"), CurrentAimRotation.Yaw);
			UE_LOG(LogTemp, Warning, TEXT("StartingAimRotation_Yaw: %f"), StartingAimRotation.Yaw);
		}*/
	}
	else if (Speed > 0.1 || bIsInAir) // running or jumping
	{
		bRotateRootBone = true;

		StartingAimRotation=FRotator(0,GetBaseAimRotation().Yaw,0);
		AO_Yaw=0;
		bUseControllerRotationYaw=true;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();

	/*
	* 通过组合这两个条件输出对应的值进行debug
	if (!HasAuthority() && !IsLocallyControlled())
	*/
}

void ABattleCharacter::CalculateAO_Pitch()
{
	// 网络传输时会被映射成[0,360],即不允许负数
	// 0 被作为分界线，0以下从360开始，即取模360
	// 压缩和解压时格式会有异常
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270,360] to [-90,0]
		AO_Pitch = UKismetMathLibrary::MapRangeClamped(AO_Pitch, 270, 360, -90, 0);
	}
}

void ABattleCharacter::SimProxiesTurn()
{
	if (CombatComponent == nullptr || CombatComponent->EquippedWeapon == nullptr) return;
	bRotateRootBone = false;

	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;
	// UE_LOG(LogTemp, Warning, TEXT("ProxyYaw: %f"), ProxyYaw);

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

// 转身动画实质上只是播放，并不改变朝向
// 通过动画的的节点rotate bone改变朝向
void ABattleCharacter::TurnInPlace(float DeltaTime)
{
	// UE_LOG(LogTemp, Warning, TEXT("AO_Yaw: %f"), AO_Yaw);
	if (AO_Yaw > 90)
	{
		TurningInPlace=ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90)
	{
		TurningInPlace=ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		Interp_AO_Yaw=FMath::FInterpTo(Interp_AO_Yaw,0,DeltaTime,3);
		AO_Yaw=Interp_AO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15)
		{
			TurningInPlace=ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		}
	}
}

void ABattleCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{// 服务器上或者自主
		AimOffset(DeltaTime);
	}
	else
	{
		//SimProxiesTurn(); // 不能在tick中调用，因为tick的频率实际上比网络netfrequence快，所以有些delta值为0
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

void ABattleCharacter::HideCharacterIfCameraClose()
{
	if(!IsLocallyControlled()) return;

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (CombatComponent && CombatComponent->EquippedWeapon)
		{
			CombatComponent->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee=true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (CombatComponent && CombatComponent->EquippedWeapon)
		{
			CombatComponent->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABattleCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();

	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

AWeapon* ABattleCharacter::GetEquippedWeapon()
{
	if(CombatComponent==nullptr) return nullptr;

	return CombatComponent->EquippedWeapon;
}

void ABattleCharacter::PlayFireMontage(bool bAiming)
{
	if(CombatComponent==nullptr || CombatComponent->EquippedWeapon==nullptr) return;
	
	UAnimInstance* AnimInstance=GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABattleCharacter::PlayHitReactMontage()
{
	if (CombatComponent == nullptr || CombatComponent->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName;
		SectionName = FName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABattleCharacter::PlayReloadMontage()
{
	if (CombatComponent == nullptr || CombatComponent->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (CombatComponent->EquippedWeapon->GetWeaponType())
		{
			case EWeaponType::EWT_AssaultRifle:
			{
				SectionName = FName("Rifle");
				break;
			}
			case EWeaponType::EWT_RocketLauncher:
			{
				SectionName = FName("RocketLauncher");
				break;
			}
			case EWeaponType::EWT_Pistol:
			{
				SectionName = FName("Pistol");
				break;
			}
			case EWeaponType::EWT_SubmachineGun:
			{
				SectionName = FName("Pistol");
				break;
			}
			case EWeaponType::EWT_Shotgun:
			{
				SectionName = FName("Shotgun");
				break;
			}
			case EWeaponType::EWT_SniperRifle:
			{
				SectionName = FName("SniperRifle");
				break;
			}
			case EWeaponType::EWT_GrenadeLauncher:
			{
				SectionName = FName("GrenadeLauncher");
				break;
			}
		}
		
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

FVector ABattleCharacter::GetHitTarget() const
{
	if (CombatComponent == nullptr) return FVector();

	return CombatComponent->HitTarget;
}

void ABattleCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

float ABattleCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0;
	return Velocity.Size();
}

void ABattleCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser)
{
	if (bElimmed) return; // 正在死亡无需再次受到伤害

	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			// 减护盾
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{	
			// 减血
			Shield = 0.f;
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
		}
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

	// 服务端的反应，在Onrep_health,对客户端进行同样的设置
	UpdateHUDHealth();
	// 服务端的反应，在Onrep_Shield,对客户端进行同样的设置
	UpdateHUDShield();
	PlayHitReactMontage();

	if (Health <= 0.1f)
	{
		ABattleGameMode* BattleGameMode = GetWorld()->GetAuthGameMode<ABattleGameMode>();
		if (BattleGameMode)
		{
			BattlePlayerController = BattlePlayerController == nullptr ? Cast<ABattlePlayerController>(Controller) : BattlePlayerController;
			ABattlePlayerController* AttackerController = Cast<ABattlePlayerController>(InstigatorController);
			BattleGameMode->PlayerEliminated(this, BattlePlayerController, AttackerController);
		}
	}
}

void ABattleCharacter::UpdateHUDHealth()
{
	BattlePlayerController = BattlePlayerController == nullptr ? Cast<ABattlePlayerController>(GetController()) : BattlePlayerController;
	if (BattlePlayerController)
	{
		BattlePlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABattleCharacter::Elim()
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		CombatComponent->EquippedWeapon->Dropped();
	}

	MulticastElim();
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABattleCharacter::ElimTimerFinished,
		ElimDelayTime);
}

void ABattleCharacter::MulticastElim_Implementation()
{
	bElimmed=true;
	PlayElimMontage();
	
	// Start Dissolve effect 
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// Disable Movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	// Disable Gameplay Input
	bDisableGameplay=true;
	if (CombatComponent)
	{
		CombatComponent->FireButtonPress(false);
	}

	// Disable Collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn Elim Bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotEffectComp = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}

	// 清空子弹数量
	if (BattlePlayerController)
	{
		BattlePlayerController->SetHUDWeaponAmmo(0);
	}

	// 如果开镜则关闭
	bool bHideSniperScope = IsLocallyControlled() &&
		CombatComponent &&
		CombatComponent->bAiming &&
		CombatComponent->EquippedWeapon &&
		CombatComponent->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}
}

void ABattleCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABattleCharacter::ElimTimerFinished()
{
	ABattleGameMode* BattleGameMode = GetWorld()->GetAuthGameMode<ABattleGameMode>();
	if (BattleGameMode)
	{
		BattleGameMode->RequestRespawn(this,GetController());
	}
}

void ABattleCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABattleCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABattleCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimelineComp)
	{
		DissolveTimelineComp->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimelineComp->Play();
	}
}

void ABattleCharacter::PollInit()
{
	if (BattlePlayerState==nullptr)
	{
		BattlePlayerState=GetPlayerState<ABattlePlayerState>();
		if (BattlePlayerState)
		{
			BattlePlayerState->AddToScore(0);
			BattlePlayerState->AddToDefeats(0);
		}
	}
}

ECombatState ABattleCharacter::GetCombatState() const
{
	if (CombatComponent == nullptr) return ECombatState::ECS_MAX;
	return CombatComponent->CombatState;
}

void ABattleCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABattleCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}

void ABattleCharacter::UpdateHUDShield()
{
	BattlePlayerController = BattlePlayerController == nullptr ? Cast<ABattlePlayerController>(Controller) : BattlePlayerController;
	if (BattlePlayerController)
	{
		BattlePlayerController->SetHUDShield(Shield, MaxShield);
	}
}

