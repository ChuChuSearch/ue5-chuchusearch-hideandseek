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
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

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

    virtual void PostSeamlessTravel() override;
    virtual void HandleSeamlessTravelPlayer(AController*& C) override;

    void StartGameTimeline();
    void EnterGamePhase(EGamePhase NewPhase, double DurationSeconds);
    void EnterPhase1();
    void EnterForcedSwap();
    void EnterPhase2();
    void EnterFeverTime();
    void EndGameByTime();
    void ResolveForcedSwapProps();
    void ClearPhase1PropBans();
    bool HasAnyActiveRunner() const;

    FTimerHandle TH_GamePhase;
    TSet<TWeakObjectPtr<APropBase>> Phase1PossessedProps;
    TSet<TWeakObjectPtr<APropBase>> BannedPhase1Props;
};
