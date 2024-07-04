#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Battle/Character/BattleCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "BulletShell.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Battle/Character/BattleCharacter.h"
#include "Battle/PlayerController/BattlePlayerController.h"


AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates=true; // 只有服务器上有一把
	SetReplicateMovement(true);

	WeaponMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);

	// 仅在服务器上开启碰撞，在BeginPlay中执行开启逻辑
	AreaSphere=CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); 
	AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);

	PickupWidget=CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(GetRootComponent());
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (PickupWidget)
	{
		ShowPickupWidget(false);
	}

	if (HasAuthority()) // 等同于：GetLocalRole() == ENetRole::ROLE_Authority,即在服务器上
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this,&AWeapon::OnSphereEndOverlap);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABattleCharacter* BattleCharacter=Cast<ABattleCharacter>(OtherActor);
	if (BattleCharacter)
	{
		BattleCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABattleCharacter* BattleCharacter = Cast<ABattleCharacter>(OtherActor);
	if (BattleCharacter)
	{
		BattleCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
	{
		ShowPickupWidget(false);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}
	case EWeaponState::EWS_Dropped:
	{
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
	}
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);

	FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld,true);
	WeaponMesh->DetachFromComponent(DetachmentTransformRules);

	SetOwner(nullptr);
	BattleOwnerCharacter=nullptr;
	BattleOwnerPlayerController=nullptr;
}


void AWeapon::SetupHUDAmmo()
{
	BattleOwnerCharacter = BattleOwnerCharacter == nullptr ? Cast<ABattleCharacter>(GetOwner()) : BattleOwnerCharacter;
	if (BattleOwnerCharacter)
	{
		BattleOwnerPlayerController = BattleOwnerPlayerController == nullptr ? Cast<ABattlePlayerController>(BattleOwnerCharacter->GetController()) : BattleOwnerPlayerController;
		if (BattleOwnerPlayerController)
		{
			BattleOwnerPlayerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::AddAmmo(int32 AddAmmo)
{
	Ammo = FMath::Clamp(Ammo+AddAmmo,0,MagCapacity);

	SetupHUDAmmo();
}

void AWeapon::SpendRound()
{
	Ammo=FMath::Clamp(Ammo-1,0,MagCapacity);

	SetupHUDAmmo();
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();

	if (Owner == nullptr)
	{
		BattleOwnerCharacter = nullptr;
		BattleOwnerPlayerController = nullptr;
	}
	else
	{
		SetupHUDAmmo();
	}
}

	

bool AWeapon::IsEmpty()
{
	return Ammo <= 0; 
}

void AWeapon::OnRep_Ammo()
{
	SetupHUDAmmo();
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::SetWeaponState(EWeaponState InWeaponState)
{
	WeaponState = InWeaponState;

	switch (WeaponState) 
	{
	case EWeaponState::EWS_Equipped:
	{
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}
	case EWeaponState::EWS_Dropped:
	{
		if (HasAuthority())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation,false);
	}

	if (BulletShellClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ABulletShell>
					(
						BulletShellClass,
						SocketTransform.GetLocation(),
						SocketTransform.GetRotation().Rotator()
					);
			}
		}
	}

	SpendRound();
}

