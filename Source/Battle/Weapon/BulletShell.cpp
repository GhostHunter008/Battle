#include "BulletShell.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ABulletShell::ABulletShell()
{
	PrimaryActorTick.bCanEverTick = false;

	ShellMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShellMesh"));
	SetRootComponent(ShellMesh);
	ShellMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,ECollisionResponse::ECR_Ignore); // 避免子弹和相机产生阻挡
	ShellMesh->SetSimulatePhysics(true);
	ShellMesh->SetEnableGravity(true);
	ShellMesh->SetNotifyRigidBodyCollision(true); // 对应blueprint:simulate generate hit event
	ShellEjectionImpulse=5;

	//SetLifeSpan(3);
}

void ABulletShell::BeginPlay()
{
	Super::BeginPlay();
	
	ShellMesh->OnComponentHit.AddDynamic(this, &ABulletShell::OnHit);
	ShellMesh->AddImpulse(GetActorForwardVector()*ShellEjectionImpulse);
}

void ABulletShell::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShellSound, GetActorLocation());
	}
	Destroy();
}

