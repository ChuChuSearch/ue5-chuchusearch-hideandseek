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
class USoundBase;

UCLASS()
class HIDE_API AMyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMyGameMode();

    // 이 게임 모드(블루프린트 기본값)의 단계 시간을 설정 구조체에 채운다(대기실 게임 설정의 처음 값)
    void FillPhaseSettings(struct FLobbyGameSettings& OutSettings) const;

    virtual void BeginPlay() override;
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

    // 접속이 끊겨도 캐릭터를 월드에 남겨 재접속 시 이어서 조종하게 할지
    bool ShouldKeepPawnOnDisconnect() const { return bAllowReconnect && !bGameEnded; }
    virtual void Logout(AController* Exiting) override;

    void SpawnPlayerAsRole(APlayerController* PC, bool bSeeker);
    void HandleRunnerEliminated(AMyCharacter* EliminatedRunner);
    // 모든 플레이어 화면에 탈락 알림. 실제 탈락 순간에 한 번만 호출(재접속 시 유령 전환에서는 호출 안 함)
    void BroadcastEliminationNotice(const AMyCharacter* EliminatedRunner) const;
    void HandleCodeVictory(EFinalRole WinningRole);
    void EndGameWithWinner(EFinalRole WinningRole);

    // 결과 화면에서 방장이 "바로 대기실로"를 누른 경우. 게임이 끝났고 요청자가 방장일 때만 이동한다.
    void RequestReturnToWaitRoom(APlayerController* Requester);

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

    // 끊긴 사람별 재접속 시간(지나면 캐릭터 정리)
    TMap<FString, FTimerHandle> PlayerReconnectTimers;
    void OnPlayerReconnectExpired(FString Key);

    // 한 팀이 접속자 0명인지
    bool IsAnyTeamMissing() const;

    // 다른 사람들에게 연결 끊김/복귀 안내
    void BroadcastConnectionNotice(const AMyPlayerState* About, EConnectionNoticeKind Kind, const FString& SubText) const;

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
    int32 CountActiveRunners() const;
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

    // ===== 점수 =====
    // 물체로 숨은 사람 주변 이 거리(cm) 안에서 술래가 볼 수 있으면 "코앞에서 숨기" 점수
    UPROPERTY(EditDefaultsOnly, Category = "Score", meta = (ClampMin = "0.0"))
    float CloseHideRadius = 500.f;

    FTimerHandle TH_CloseHideScore;
    void TickCloseHideScore();
    void AwardWinBonus(EFinalRole WinningRole);

    // 게임 중에는 상대 팀 점수를 숨기므로, 팀원 점수는 같은 팀 플레이어에게만 보낸다
    FTimerHandle TH_TeamScores;
    void PushTeamScores();

    // ===== 시간 끌기 방지 =====
    // 피버 타임 동안 살아 있는 숨는 사람 위치에서 주기적으로 소리를 낸다(멀리 도망가 버티기 방지)
    UPROPERTY(EditDefaultsOnly, Category = "Anti Stall")
    TObjectPtr<USoundBase> StallTauntSound;

    UPROPERTY(EditDefaultsOnly, Category = "Anti Stall", meta = (ClampMin = "1.0"))
    float StallTauntIntervalSeconds = 10.f;

    // 피버 타임 시작 후 첫 소리까지(초)
    UPROPERTY(EditDefaultsOnly, Category = "Anti Stall", meta = (ClampMin = "0.0"))
    float StallTauntFirstDelaySeconds = 3.f;

    FTimerHandle TH_StallTaunt;
    void PlayStallTaunts();
};
