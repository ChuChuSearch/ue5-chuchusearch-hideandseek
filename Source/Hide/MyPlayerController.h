#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "MyPlayerState.h"
#include "MyPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class AMyCharacter;
class APropBase;
class AGhostCharacter;
class AMyPlayerState;
class APasswordDoor;

UCLASS()
class HIDE_API AMyPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMyPlayerController();

    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;

    UFUNCTION(BlueprintCallable)
    void UI_RequestStartGame();

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_Attack = nullptr;

    UFUNCTION(BlueprintCallable)
    void UI_SelectGhostRespawnIndex(int32 InIndex);

    UFUNCTION(BlueprintCallable)
    void UpdateGameRoomHearts(int32 InHitCount);

    UFUNCTION(BlueprintCallable)
    void UpdateGameRoomClues(const TArray<int32>& InCollectedClueNumbers);

    UFUNCTION(BlueprintCallable)
    void UpdateGhostUI();

    UFUNCTION(BlueprintCallable)
    void UI_SubmitPasswordCode(const TArray<int32>& InputCode);

    UFUNCTION(BlueprintCallable)
    void UI_ClosePasswordInput();

    UFUNCTION(BlueprintCallable)
    bool IsGhostUIOpen() const { return GhostUIWidgetInstance != nullptr; }
    
    UFUNCTION(BlueprintCallable)
    void HideGameRoomUI();

    virtual void OnRep_Pawn() override;

    UFUNCTION(Client, Reliable)
    void ClientFinalizeGhostTransition();

    UFUNCTION(Client, Reliable)
    void ClientSetForcedSwapWarning(bool bShowWarning, double RemainingSeconds);

    UFUNCTION(Client, Reliable)
    void ClientShowGameResult(EFinalRole WinningRole);

    UFUNCTION(BlueprintImplementableEvent, Category = "Game Result")
    void OnGameResultReceived(EFinalRole WinningRole, bool bWon);
protected:
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputMappingContext* IMC_Default = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_Possess = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_Lock = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> WBP_WaitRoom = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_GhostPlacementRefresh = nullptr;

    APropBase* GetAimPropUnderCrosshair() const;
    bool GetScreenRayHit(FHitResult& OutHit, bool bPreferCursor) const;

private:
    void OnPossessPressed(const FInputActionValue& Value);
    void OnLockPressed(const FInputActionValue& Value);
    void OnGhostPlacementRefreshPressed(const FInputActionValue& Value);

    void UpdateHovered();
    void ApplyOutline(AActor* NewHovered);
    void ClearOutline(AActor* ActorToClear);

    AMyCharacter* GetMyCharacter() const;
    APropBase* GetHoveredProp() const;
    APasswordDoor* GetHoveredPasswordDoor() const;
    bool TryOpenPasswordInput();
    void UpdatePasswordInputState();
    void BroadcastPasswordCooldownUpdated(double CooldownRemaining);

    UFUNCTION(Server, Reliable)
    void ServerSubmitPasswordCode(const TArray<int32>& InputCode);

    UFUNCTION(Client, Reliable)
    void ClientReceivePasswordAttemptResult(int32 CorrectCount, bool bSuccess, double CooldownRemaining);

    UPROPERTY()
    AActor* HoveredActor = nullptr;

    UPROPERTY()
    AActor* OutlinedActor = nullptr;

    UPROPERTY()
    UUserWidget* WaitRoomWidgetInstance = nullptr;

    void ShowWaitRoomUI();

    UFUNCTION(Server, Reliable)
    void ServerRequestStartGame();

    UFUNCTION(Client, Reliable)
    void ClientCloseWaitRoomUI();

    void CloseWaitRoomUI();
    void OnAttackPressed(const FInputActionValue& Value);
    void OnAttackReleased(const FInputActionValue& Value);
    void HandleAttackHoldTriggered();

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    float AttackHoldThreshold = 0.25f; // 이 시간 이상 누르면 "오브젝트 파괴"로 판정

    FTimerHandle TH_AttackHold;
    bool bHoldTriggered = false;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> WBP_GameRoom = nullptr;

    UPROPERTY()
    UUserWidget* GameRoomWidgetInstance = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> WBP_PasswordInput = nullptr;

    UPROPERTY()
    UUserWidget* PasswordInputWidgetInstance = nullptr;

    UPROPERTY()
    APasswordDoor* ActivePasswordDoor = nullptr;

    void ShowGameRoomUI();
    void CloseGameRoomUI();
    void ShowPasswordInputUI();
    void ClosePasswordInputUI();
    void RefreshGameRoomCluesFromPlayerState();

    AGhostCharacter* GetGhostCharacter() const;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> WBP_GhostUI = nullptr;

    UPROPERTY()
    UUserWidget* GhostUIWidgetInstance = nullptr;
    void ShowGhostUI();

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> WBP_GameResult = nullptr;

    UPROPERTY()
    UUserWidget* GameResultWidgetInstance = nullptr;
};


