// Fill out your copyright notice in the Description page of Project Settings.


#include "BattleHUD.h"

void ABattleHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter(ViewportSize.X/2.0, ViewportSize.Y / 2.0);

		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		if (HUDPackage.CrosshairsCenter)
		{
			FVector2D Spread(0,0);
			DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter,Spread);
		}
		if (HUDPackage.CrosshairsTop)
		{
			FVector2D Spread(0, SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter,Spread);
		}
		if (HUDPackage.CrosshairsBottom)
		{
			FVector2D Spread(0, -SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, Spread);
		}
		if (HUDPackage.CrosshairsLeft)
		{
			FVector2D Spread(-SpreadScaled, 0);
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, Spread);
		}
			
		if (HUDPackage.CrosshairsRight)
		{
			FVector2D Spread(SpreadScaled, 0);
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, Spread);
		}
			
	}

}

void ABattleHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	const FVector2D TextureDrawPoint  // 真正渲染的位置
	(
		ViewportCenter.X-(TextureWidth/2.0) + Spread.X,
		ViewportCenter.Y-(TextureHeight/2.0 + Spread.Y)
	);

	// DrawTexture:HUD中本身就有的函数
	DrawTexture
	(
		Texture,
		TextureDrawPoint.X, 
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0,0,
		1,1,
		FLinearColor::White
	);

}
