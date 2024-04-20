#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "BattleHUD.generated.h"

// 自定义的结构体
USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	class UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;

	float CrosshairSpread;
	FLinearColor CrosshairsColor;
};

UCLASS()
class BATTLE_API ABattleHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:
	virtual void DrawHUD() override; // Call Every Frame

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package){HUDPackage=Package;}

private:
	FHUDPackage HUDPackage;

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax=16.0f; // 调整准星扩散的缩放比例

	void DrawCrosshair(UTexture2D* Texture,FVector2D ViewportCenter,FVector2D Spread,FLinearColor CrosshairsColor); // 水平和垂直扩散


public:
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY(EditAnywhere,Category="Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	void AddCharacterOverlay();
	
};
