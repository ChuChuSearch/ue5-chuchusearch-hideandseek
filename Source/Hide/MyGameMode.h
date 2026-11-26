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
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

    // 접속이 끊겨도 캐릭터를 월드에 남겨 재접속 시 이어서 조종하게 할지
    bool ShouldKeepPawnOnDisconnect() const { return bAllowReconnect && !bGameEnded; }
    virtual void Logout(AController* Exiting) override;

    void SpawnPlayerAsRole(APlayerController* PC, bool bSeeker);
    void HandleRunnerEliminated(AMyCharacter* EliminatedRunner);
    void HandleCodeVictory(EFinalRole WinningRole);
    void EndGameWithWinner(EFinalRole WinningRole);

    // 탈락으로 게임이 끝날 때 탈락 소리/화면 효과를 볼 수 있게 결과를 잠깐 늦춘다
    void EndGameWithWinnerDelayed(EFinalRole WinningRole, float DelaySeconds);
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

    // ===== 재접속 =====
    // 게임 도중 끊긴 플레이어가 같은 PC(같은 온라인 ID)로 다시 들어오면 역할/단서/캐릭터를 복원한다.
    UPROPERTY(EditDefaultsOnly, Category = "Reconnect")
    bool bAllowReconnect = true;

    // 한 팀이 전원 끊겼을 때 바로 승패를 내지 않고 재접속을 기다리는 시간(초). 0이면 즉시 판정.
    UPROPERTY(EditDefaultsOnly, Category = "Reconnect", meta = (ClampMin = "0.0"))
    float ReconnectGraceSeconds = 60.f;

    struct FDisconnectedPlayerRecord
    {
        FReconnectPlayerData Data;
        TWeakObjectPtr<APawn> Pawn;
    };

    TMap<FString, FDisconnectedPlayerRecord> DisconnectedPlayers;
    FTimerHandle TH_ReconnectGrace;

    static FString MakeReconnectKey(const FUniqueNetIdRepl& UniqueId, const FString& Address);
    FString GetReconnectKey(const APlayerController* PC) const;
    bool IsDisconnectedPlayerPawn(const APawn* Pawn) const;
    bool TryRestoreReconnectingPlayer(APlayerController* PC);
    void OnReconnectGraceExpired();

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
    void EvaluateTeamsAfterLogout(const AMyPlayerState* ExitingPlayerState, bool bAllowGrace = true);

    FTimerHandle TH_GamePhase;
    FTimerHandle TH_PendingResult;

    // 마지막 러너 탈락 후 결과 화면까지 기다리는 시간(초). 0이면 즉시.
    UPROPERTY(EditDefaultsOnly, Category = "Victory", meta = (ClampMin = "0.0"))
    float EliminationResultDelaySeconds = 2.0f;

    // 결과 대기 중에는 다른 종료(시간 만료 등)가 끼어들지 않게 막는다
    bool bResultPending = false;
    EFinalRole PendingWinner = EFinalRole::Seeker;
    void FinishPendingResult();
    FTimerHandle TH_ReturnToWaitRoom;
    bool bReturningToWaitRoom = false;
    TSet<TWeakObjectPtr<APropBase>> Phase1PossessedProps;
    TSet<TWeakObjectPtr<APropBase>> BannedPhase1Props;
};
