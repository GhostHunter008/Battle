#pragma once


UENUM(BlueprintType)
enum class ETurningInPlace :uint8
{
	ETIP_Left UMETA(DisplayName = "Turning_Left"),
	ETIP_Right UMETA(DisplayName = "Turning_Right"),
	ETIP_NotTurning UMETA(DisplayName = "Not_Turning"),

	ETIP_MAX UMETA(DisplayName = "DefaultMAX")
};