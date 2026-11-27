#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MyPlayerState.generated.h"

UENUM(BlueprintType)
enum class ERolePreference : uint8
{
    SeekerWish UMETA(DisplayName = "SeekerWish"),
    RunnerWish UMETA(DisplayName = "RunnerWish"),
    Any UMETA(DisplayName = "Any"),
};

UENUM(BlueprintType)
enum class EFinalRole : uint8
{
    None UMETA(DisplayName = "None"),
    Seeker UMETA(DisplayName = "Seeker"),
    Runner UMETA(DisplayName = "Runner"),
};

// 점수를 얻는 행동. 점수와 상한은 AMyPlayerState::AddScore_Server에서 정한다.
enum class EScoreReason : uint8
{
    SeekerHit,       // 술래: 숨는 사람 적중
    SeekerEliminate, // 술래: 숨는 사람 탈락시킴
    ClueCollected,   // 단서 획득
    PasswordSolved,  // 비밀번호 맞힘
    CloseHide,       // 숨는 사람: 술래 코앞에서 숨어 있기(1초마다)
    NearMiss,        // 숨는 사람: 술래 투사체가 가까이 빗나감
    DecoyHit,        // 유령: 놓은 물체를 술래가 쏨
    Win,             // 이긴 팀
};

// 판마다 새로 시작하는 점수와 기록(점수판에 보여 준다)
USTRUCT(BlueprintType)
struct FHideScoreStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 Score = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Hits = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Eliminations = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Clues = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bPasswordSolved = false;

    UPROPERTY(BlueprintReadOnly)
    int32 CloseHidePoints = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 NearMisses = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DecoyHits = 0;
};

// 서버가 같은 팀 플레이어에게만 보내는 팀원 점수 한 줄
USTRUCT()
struct FTeamScoreEntry
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<APlayerState> Player = nullptr;

    UPROPERTY()
    FHideScoreStats Stats;
};

// 게임 도중 접속이 끊긴 플레이어를 재접속 시 복원하기 위한 데이터(서버 전용)
struct FReconnectPlayerData
{
    FString Nickname;
    ERolePreference Preference = ERolePreference::Any;
    EFinalRole FinalRole = EFinalRole::None;
    bool bIsHost = false;
    int32 LobbyJoinOrder = 0;
    TArray<int32> CollectedClueNumbers;
    TArray<int32> PositionedClueNumbers;
    FHideScoreStats ScoreStats;
    bool bRoundEliminated = false;
};

UCLASS()
class HIDE_API AMyPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AMyPlayerState();

    UFUNCTION(BlueprintCallable)
    const FString& GetNickname() const { return Nickname; }

    UFUNCTION(BlueprintCallable)
    ERolePreference GetPreference() const { return Preference; }

    UFUNCTION(BlueprintCallable)
    EFinalRole GetFinalRole() const { return FinalRole; }

    UFUNCTION(BlueprintCallable)
    bool IsHost() const { return bIsHost; }

    UFUNCTION(BlueprintCallable)
    int32 GetLobbyJoinOrder() const { return LobbyJoinOrder; }

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void ServerSetNickname(const FString& InNickname);

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void ServerSetPreference(ERolePreference InPreference);

    // 대기실 준비(방장은 준비 없이 시작 버튼을 누른다). 대기실에 올 때마다 새 PlayerState라 자동으로 풀린다.
    UFUNCTION(BlueprintCallable)
    bool IsReady() const { return bReady; }

    UFUNCTION(Server, Reliable)
    void ServerSetReady(bool bNewReady);

    bool IsInWaitRoom_Server() const;

    void ExportReconnectData(FReconnectPlayerData& OutData) const;
    void ImportReconnectData_Server(const FReconnectPlayerData& InData);

    void SetHost_Server(bool bNewHost);
    void SetLobbyJoinOrder_Server(int32 NewJoinOrder);
    void SetFinalRole_Server(EFinalRole InRole);

    virtual void CopyProperties(APlayerState* PlayerState) override;
    virtual void OverrideWith(APlayerState* PlayerState) override;

    UFUNCTION(BlueprintCallable)
    bool HasCollectedClue(int32 InClueNumber) const;

    UFUNCTION(BlueprintCallable)
    int32 GetCollectedClueCount() const { return CollectedClueNumbers.Num(); }

    UFUNCTION(BlueprintCallable)
    const TArray<int32>& GetCollectedClueNumbers() const { return CollectedClueNumbers; }

    // 서버 전용. 클라이언트 RPC로 열어두면 단서를 줍지 않고도 수집 처리할 수 있으므로
    // ACluePickup의 서버측 겹침 판정에서만 호출한다.
    // 새로 얻은 단서면 true(이미 가진 단서/잘못된 번호면 false)
    bool CollectClue_Server(int32 InClueNumber);

    // 자리(순서)가 반영된 단서 목록. 서버가 계산해 소유자에게만 복제한다.
    const TArray<int32>& GetPositionedClueNumbers() const { return PositionedClueNumbers; }

    // ===== 점수(판마다 새로 시작) =====
    // 상대 팀이 점수 변화로 위치를 짐작하지 못하게(예: "코앞" 점수가 오르면 근처에 숨은 사람이 있다)
    // 게임 중 점수는 본인에게만 복제하고, 팀원 점수는 서버가 같은 팀에게만 RPC로 보낸다.
    // 게임이 끝나면 RevealScores_Server로 모두에게 공개한다.

    // 서버에서는 실제 점수, 클라이언트에서는 본인 것만 정확하다(다른 사람은 UI에서 쓰지 말 것)
    const FHideScoreStats& GetScoreStats() const { return ScoreStats; }

    // 게임이 끝난 뒤 모두에게 공개된 점수
    bool AreScoresRevealed() const { return bScoresRevealed; }
    const FHideScoreStats& GetRevealedScoreStats() const { return RevealedScoreStats; }
    void RevealScores_Server();

    // 서버 전용. 상한에 걸리거나 이미 공개된 뒤면 false.
    bool AddScore_Server(EScoreReason Reason);

    // 이번 판에 탈락해서 유령이 됐는지(모두에게 공개: 탈락 알림으로 이미 알려지는 정보)
    bool IsRoundEliminated() const { return bRoundEliminated; }
    void MarkEliminated_Server();

    // 점수 규칙(도움말 UI와 서버가 같은 값을 쓴다)
    static constexpr int32 ScoreSeekerHit = 50;
    static constexpr int32 ScoreSeekerEliminate = 100;
    static constexpr int32 ScoreClue = 100;
    static constexpr int32 ScorePassword = 300;
    static constexpr int32 ScoreCloseHidePerSecond = 5;
    static constexpr int32 MaxCloseHidePoints = 150;
    static constexpr int32 ScoreNearMiss = 20;
    static constexpr int32 MaxNearMisses = 5;
    static constexpr int32 ScoreDecoyHit = 30;
    static constexpr int32 MaxDecoyHits = 10;
    static constexpr int32 ScoreWin = 200;

protected:
    UPROPERTY(Replicated)
    FHideScoreStats ScoreStats;

    UPROPERTY(Replicated)
    FHideScoreStats RevealedScoreStats;

    UPROPERTY(Replicated)
    bool bScoresRevealed = false;

    UPROPERTY(Replicated)
    bool bRoundEliminated = false;

    UPROPERTY(Replicated)
    bool bReady = false;

    UPROPERTY(Replicated)
    FString Nickname;

    UPROPERTY(Replicated)
    ERolePreference Preference = ERolePreference::Any;

    UPROPERTY(Replicated)
    EFinalRole FinalRole = EFinalRole::None;

    UPROPERTY(Replicated)
    bool bIsHost = false;

    UPROPERTY(Replicated)
    int32 LobbyJoinOrder = 0;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(ReplicatedUsing = OnRep_CollectedClueNumbers)
    TArray<int32> CollectedClueNumbers;

    UFUNCTION()
    void OnRep_CollectedClueNumbers();

    UPROPERTY(ReplicatedUsing = OnRep_PositionedClueNumbers)
    TArray<int32> PositionedClueNumbers;

    UFUNCTION()
    void OnRep_PositionedClueNumbers();

    void NotifyClueCollectionChanged();
};
