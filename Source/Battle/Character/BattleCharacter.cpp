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

ABattleCharacter::ABattleCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	NetUpdateFrequency=66;
	MinNetUpdateFrequency=33;

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
	CombatComponent->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch=true; //赋予蹲的能力
	GetCharacterMovement()->RotationRate.Yaw=850; // 转身的速度
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	TurningInPlace=ETurningInPlace::ETIP_NotTurning;
}

void ABattleCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ABattleCharacter,OverlappingWeapon);
	DOREPLIFETIME_CONDITION(ABattleCharacter, OverlappingWeapon,COND_OwnerOnly);
}

void ABattleCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (CombatComponent)
	{
		CombatComponent->BattleCharacter=this;
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
	
}

void ABattleCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
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
	}
}

void ABattleCharacter::Move(const FInputActionValue& Value)
{
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
	if (bIsCrouched)
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
	if (CombatComponent)
	{
		CombatComponent->SetAiming(true);
	}
}

void ABattleCharacter::AimButtonRelease(const FInputActionValue& Value)
{
	if (CombatComponent)
	{
		CombatComponent->SetAiming(false);
	}
}

void ABattleCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
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



void ABattleCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
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
	
	FVector Velocity = GetVelocity();
	Velocity.Z = 0;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed <= 0.1 && !bIsInAir) // standing still
	{

		FRotator CurrentAimRotation= FRotator(0, GetBaseAimRotation().Yaw, 0);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw=DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			Interp_AO_Yaw=AO_Yaw;
		}
		bUseControllerRotationYaw=true;

		TurnInPlace(DeltaTime);

		/*
		* 注意：此处让busecontroller为false，那么从服务器到客户端的yaw的复制是有问题的
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
		StartingAimRotation=FRotator(0,GetBaseAimRotation().Yaw,0);
		AO_Yaw=0;
		bUseControllerRotationYaw=true;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	// 网络传输时会被映射成[0,360],即不允许负数
	// 压缩和解压时格式会有异常
	AO_Pitch=GetBaseAimRotation().Pitch;  
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270,360] to [-90,0]
		AO_Pitch=UKismetMathLibrary::MapRangeClamped(AO_Pitch,270,360,-90,0);
	}

	/*
	* 通过组合这两个条件输出对应的值进行debug
	if (!HasAuthority() && !IsLocallyControlled())
	*/
}

void ABattleCharacter::TurnInPlace(float DeltaTime)
{
	UE_LOG(LogTemp, Warning, TEXT("AO_Yaw: %f"), AO_Yaw);
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

AWeapon* ABattleCharacter::GetEquippedWeapon()
{
	if(CombatComponent==nullptr) return nullptr;

	return CombatComponent->EquippedWeapon;
}

