#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Battle/Battle.h"
#include "LagCompensationComponent.generated.h"


USTRUCT(BlueprintType)
struct FBoxInformation  // 用于存储BoxComponent的位置信息
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage  // 用于服务端记录的相关时间的盒体记录
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
	ABattleCharacter* Character; // 为霰弹枪逻辑服务(实际上不需要这么改，可以优化逻辑)
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult  // 服务端返回的确认结果
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed = false;

	UPROPERTY()
	bool bHeadShot = false;
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult  // 服务端返回的确认结果，霰弹枪特制
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<ABattleCharacter*, uint32> HeadShots;

	UPROPERTY()
	TMap<ABattleCharacter*, uint32> BodyShots;

};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BATTLE_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	friend class ABattleCharacter;
	ULagCompensationComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

public:
	
	// 1p调用的函数 （服务端没必要倒带）
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
		ABattleCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime,
		class AWeapon* DamageCauser
	);

	// 1p调用的函数,霰弹枪特制 （服务端没必要倒带）
	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(
		const TArray<ABattleCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime
	);

	// 1p调用的函数,projectile 类武器特制 （服务端没必要倒带）
	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(
		ABattleCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	TDoubleLinkedList<FFramePackage> FrameHistory; // 双链表,头部是最新帧，尾部是旧的帧

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f; // 服务器端最多记录的时间

	void SaveFramePackages(); // 仅在服务端调用,记录玩家一段时间内的frame
	void SaveFramePackage(FFramePackage& Package); 
	
	FServerSideRewindResult ServerSideRewind( // 服务器倒带，返回确认结果
		class ABattleCharacter* HitCharacter, 
		const FVector_NetQuantize& TraceStart, // 单单仅有一个hitlocation，有可能该点在碰撞的表面，无法检测出来
		const FVector_NetQuantize& HitLocation, 
		float HitTime); 

	FFramePackage GetFrameToCheck(ABattleCharacter* HitCharacter, float HitTime); // 根据HitCharacter和HitTime计算获得服务端需要检查的帧

	FServerSideRewindResult ConfirmHit( // 检测盒体的命中结果
		const FFramePackage& Package,
		ABattleCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation); // 单单仅有一个hitlocation，有可能该点在碰撞的表面，无法检测出来

	// 找到currenttime相邻的两帧，根据时间进行插值得到新帧
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime); 

	void CacheBoxPositions(ABattleCharacter* HitCharacter, FFramePackage& OutFramePackage); // 将HitCharacter的盒子信息保存在OutFramePackage中

	void MoveBoxes(ABattleCharacter* HitCharacter, const FFramePackage& Package);  // 将HitCharacter中的盒子位置移动到Package信息中的位置

	void ResetHitBoxes(ABattleCharacter* HitCharacter, const FFramePackage& Package); // 将HitCharacter中的盒子位置移动到Package信息中的位置（且重置盒子碰撞为无碰撞）

	void EnableCharacterMeshCollision(ABattleCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);

	void ShowFramePackage(const FFramePackage& Package, const FColor& Color); //  For debug

	/**
	* Shotgun
	*/
	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		const TArray<ABattleCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime);

	FShotgunServerSideRewindResult ShotgunConfirmHit(
		const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations
	);

	/*
	* Projectile
	*/
	FServerSideRewindResult ProjectileServerSideRewind(
		ABattleCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	FServerSideRewindResult ProjectileConfirmHit(
		const FFramePackage& Package,
		ABattleCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

public:	
	// 在character的PostInitializeComponents()中初始化
	UPROPERTY()
	class ABattleCharacter* Character;
	UPROPERTY()
	class ABattlePlayerController* Controller;
	
		
};
