#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BattleHUD.generated.h"

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
};

UCLASS()
class BATTLE_API ABattleHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void DrawHUD() override; // Call Every Frame

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package){HUDPackage=Package;}

private:
	FHUDPackage HUDPackage;

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax=16.0f; // 调整缩放的比例

	void DrawCrosshair(UTexture2D* Texture,FVector2D ViewportCenter,FVector2D Spread);
	
};
