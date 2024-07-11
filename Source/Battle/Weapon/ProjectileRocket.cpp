#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "RocketMovementComponent.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->InitialSpeed = 3000;
	RocketMovementComponent->MaxSpeed = 3000;
	RocketMovementComponent->ProjectileGravityScale = 0;
	RocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay(); // 在服务端已经绑定了OnHit

	if (!HasAuthority()) // 客户端绑定OnHit
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}

	SpawnTrailSystem(); // 播放尾迹特效

	if (ProjectileLoop && LoopingSoundAttenuation) // 播放声音特效
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			LoopingSoundAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 避免在发射过程中碰到自己
	if (OtherActor == GetOwner())
	{
		return;
	}
	ExplodeDamage();

	// 火箭筒的部分效果想要延迟销毁，故不调用父类OnHit函数。如尾迹烟雾
	// Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
	
	StartDestroyTimer();  // 延迟销毁

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstanceController())
	{
		TrailSystemComponent->GetSystemInstanceController()->Deactivate();
	}
	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}

void AProjectileRocket::Destroyed() // 避免调用父类的函数
{

}
