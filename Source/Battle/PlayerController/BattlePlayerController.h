#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BattlePlayerController.generated.h"


UCLASS()
class BATTLE_API ABattlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SetHUDHealth(float Health,float MaxHealth);

	virtual void OnPossess(APawn* InPawn) override;
	
protected:
	virtual void BeginPlay() override;

private:
	class ABattleHUD* BattleHUD;
};
