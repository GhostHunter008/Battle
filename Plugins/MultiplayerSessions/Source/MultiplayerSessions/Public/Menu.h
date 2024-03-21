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
	// 在蓝图中调用，创建此控件时调用该函数
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 InNumPublicConnections = 12, FString InMatchType = TEXT("FreeForAll"),FString InPathToLobby= TEXT("/Game/ThirdPerson/Maps/Lobby"));

protected:
	
	virtual bool Initialize() override;

	virtual void NativeDestruct() override;

	// 菜单关闭时的清理工作
	void MenuTearDown();

	/*
	* 绑定到MultiplayerSessionSubsystem中自定义委托的回调函数
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
	class UButton* HostButton; 
	
	UPROPERTY(meta = (BindWidget))
	class UButton* JoinButton;

	UFUNCTION()
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();

	// 对OnlineSession的所有功能做了封装
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	int32 NumPublicConnections={4};
	FString MatchType={TEXT("FreeForAll")};
	FString PathToLobby{TEXT("")};
};
