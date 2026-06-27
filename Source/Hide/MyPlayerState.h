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

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerCollectClue(int32 InClueNumber);

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

    void NotifyClueCollectionChanged();
};
