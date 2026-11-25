#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "MyGameState.h"
#include "MyPlayerState.h"
#include "MyGameMode.generated.h"

class AMyCharacter;
class APlayerController;
class AGhostCharacter;
class APropBase;

UCLASS()
class HIDE_API AMyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMyGameMode();

    virtual void BeginPlay() override;
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

    void SpawnPlayerAsRole(APlayerController* PC, bool bSeeker);
    void HandleRunnerEliminated(AMyCharacter* EliminatedRunner);
    void HandleCodeVictory(EFinalRole WinningRole);
    void EndGameWithWinner(EFinalRole WinningRole);
    void RegisterPhase1PossessedProp(APropBase* Prop);
    bool IsPhase1PossessedProp(const APropBase* Prop) const;

    UFUNCTION(BlueprintCallable, Category = "Victory")
    void NotifyRunnerReachedSpecialExit(AMyCharacter* Runner);

    UFUNCTION(BlueprintImplementableEvent, Category = "Victory")
    void OnCodeVictory(EFinalRole WinningRole);

    UFUNCTION(BlueprintImplementableEvent, Category = "Game Phase")
    void OnGamePhaseChanged(EGamePhase NewPhase);

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Spawn")
    TSubclassOf<AMyCharacter> PlayerCharacterClass = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Spawn")
    TSubclassOf<AGhostCharacter> GhostCharacterClass = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Spawn")
    bool bAutoSpawnOnLogin = true;

    // 게임 맵은 대기실에서 seamless travel로 넘어온 플레이어만 받는다.
    // 새 접속(PreLogin)은 거부. PIE 테스트(게임 맵 바로 실행)에서는 적용하지 않음.
    UPROPERTY(EditDefaultsOnly, Category = "Spawn")
    bool bRejectJoinInProgress = true;

    bool IsPlayInEditorWorld() const;

    UPROPERTY(BlueprintReadOnly, Category = "Victory")
    bool bGameEnded = false;

    UPROPERTY(EditDefaultsOnly, Category = "Game Phase")
    bool bAutoStartGameTimeline = true;

    UPROPERTY(EditDefaultsOnly, Category = "Game Phase")
    double HideTimeSeconds = 30.0;

    UPROPERTY(EditDefaultsOnly, Category = "Game Phase")
    double Phase1Seconds = 240.0;

    UPROPERTY(EditDefaultsOnly, Category = "Game Phase")
    double ForcedSwapSeconds = 30.0;

    UPROPERTY(EditDefaultsOnly, Category = "Game Phase")
    double Phase2Seconds = 240.0;

    UPROPERTY(EditDefaultsOnly, Category = "Game Phase")
    double FeverTimeSeconds = 60.0;

    UPROPERTY(EditDefaultsOnly, Category = "Forced Swap")
    TArray<TSubclassOf<APropBase>> ForcedSwapPropClasses;

    UPROPERTY(EditDefaultsOnly, Category = "Victory")
    int32 RequiredCluesForRunnerExit = 3;

    UPROPERTY(EditDefaultsOnly, Category = "Testing|Return To Wait Room")
    bool bAutoReturnToWaitRoom = true;

    UPROPERTY(EditDefaultsOnly, Category = "Testing|Return To Wait Room", meta = (ClampMin = "0.0"))
    float ReturnToWaitRoomDelaySeconds = 10.f;

    UPROPERTY(EditDefaultsOnly, Category = "Testing|Return To Wait Room")
    FString WaitRoomTravelURL =
        TEXT("/Game/Game/Levels/WaitRoomMap?game=/Game/Game/Blueprints/Framework/GameMode/BP_WaitRoomGameMode.BP_WaitRoomGameMode_C");

    virtual void PostSeamlessTravel() override;
    virtual void HandleSeamlessTravelPlayer(AController*& C) override;

    void StartGameTimeline();
    void EnterGamePhase(EGamePhase NewPhase, double DurationSeconds);
    void EnterPhase1();
    void EnterForcedSwap();
    void EnterPhase2();
    void EnterFeverTime();
    void EndGameByTime();
    void ReturnToWaitRoom();
    void ResolveForcedSwapProps();
    void ClearPhase1PropBans();
    bool HasAnyActiveRunner() const;
    void EvaluateTeamsAfterLogout(const AMyPlayerState* ExitingPlayerState);

    FTimerHandle TH_GamePhase;
    FTimerHandle TH_ReturnToWaitRoom;
    bool bReturningToWaitRoom = false;
    TSet<TWeakObjectPtr<APropBase>> Phase1PossessedProps;
    TSet<TWeakObjectPtr<APropBase>> BannedPhase1Props;
};
