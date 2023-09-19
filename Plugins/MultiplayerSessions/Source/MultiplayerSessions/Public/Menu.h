// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 InNumPublicConnections = 4, FString InMatchType = TEXT("FreeForAll"),FString InPathToLobby= TEXT("/Game/ThirdPerson/Maps/Lobby"));

protected:
	virtual bool Initialize() override;
	
	//virtual void OnLevelRemovedFromWorld(ULevel* InLevel,UWorld* InWorld) override;
	virtual void NativeDestruct() override;

	/*
	* callbacks for the delegate on the MultiplayerSessionSubsystem
	* 动态多播必须要UFUNCTION，其可序列化
	*/
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);


private:
	
	UPROPERTY(meta=(BindWidget))
	class UButton* HostButton; // must be the same name to the blueprint
	
	UPROPERTY(meta = (BindWidget))
	class UButton* JoinButton;

	UFUNCTION()
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();

	void MenuTearDown();

	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	int32 NumPublicConnections={4};
	FString MatchType={TEXT("FreeForAll")};
	FString PathToLobby{TEXT("")};
};
