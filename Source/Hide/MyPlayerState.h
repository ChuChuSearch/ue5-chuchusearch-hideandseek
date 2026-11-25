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
    void CollectClue_Server(int32 InClueNumber);

    // 자리(순서)가 반영된 단서 목록. 서버가 계산해 소유자에게만 복제한다.
    const TArray<int32>& GetPositionedClueNumbers() const { return PositionedClueNumbers; }

protected:
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
