#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MyPlayerState.h"
#include "MyGameState.generated.h"

class ACluePickup;
class APropBase;
class APlayerController;

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    None UMETA(DisplayName = "None"),
    HideTime UMETA(DisplayName = "HideTime"),
    Phase1 UMETA(DisplayName = "Phase1"),
    ForcedSwap UMETA(DisplayName = "ForcedSwap"),
    Phase2 UMETA(DisplayName = "Phase2"),
    FeverTime UMETA(DisplayName = "FeverTime"),
    Ended UMETA(DisplayName = "Ended"),
};

USTRUCT(BlueprintType)
struct FRespawnPropInfo
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<AActor> PropClass;

    UPROPERTY(BlueprintReadOnly)
    FTransform Transform;
};

UCLASS()
class HIDE_API AMyGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AMyGameState();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable)
    void RegisterDestroyedProp(AActor* DestroyedProp);

    UFUNCTION(BlueprintCallable)
    const TArray<FRespawnPropInfo>& GetRespawnList() const { return VisibleRespawnList; }

    bool IsValidRespawnIndex(int32 Index) const;
    bool ConsumeRespawnEntry(int32 Index, FRespawnPropInfo& OutInfo);

    void RestoreRespawnEntry(const FRespawnPropInfo& Info);

    UFUNCTION(BlueprintCallable)
    bool RequestRefreshVisibleRespawnList();

    UFUNCTION(BlueprintCallable)
    bool CanRefreshVisibleRespawnList() const;

    UFUNCTION(BlueprintCallable)
    bool DoesPropHaveClue(const FName& PropId) const;

    UFUNCTION(BlueprintCallable)
    bool TrySpawnClueFromDestroyedProp(APropBase* DestroyedProp);

    UFUNCTION(BlueprintCallable)
    bool GetClueNumberByPropId(const FName& PropId, int32& OutClueNumber) const;

    UFUNCTION(BlueprintCallable)
    double GetRunnerCodeInputCooldownRemaining() const;

    UFUNCTION(BlueprintCallable)
    double GetCodeInputCooldownRemaining(EFinalRole FinalRole) const;

    UFUNCTION(BlueprintCallable)
    EGamePhase GetGamePhase() const { return CurrentGamePhase; }

    UFUNCTION(BlueprintCallable)
    double GetPhaseRemainingSeconds() const;

    UFUNCTION(BlueprintCallable)
    bool IsFeverTime() const { return CurrentGamePhase == EGamePhase::FeverTime; }

    void SetGamePhase_Server(EGamePhase NewPhase, double DurationSeconds);

    bool SubmitPasswordCode(APlayerController* RequestPC, const TArray<int32>& InputCode, int32& OutCorrectCount, double& OutCooldownRemaining);

    UFUNCTION(BlueprintCallable)
    int32 CountRunnerTeamCollectedClues() const;

    UFUNCTION(BlueprintCallable, Category = "Team Status")
    void GetActiveTeamCounts(int32& OutSeekerCount, int32& OutRunnerCount) const;

protected:
    UPROPERTY(Replicated, BlueprintReadOnly)
    TArray<FRespawnPropInfo> RespawnList;

    UPROPERTY(Replicated, BlueprintReadOnly)
    TArray<FRespawnPropInfo> VisibleRespawnList;

    UPROPERTY(Replicated)
    TArray<int32> VisibleRespawnIndices;

    UPROPERTY(Replicated, BlueprintReadOnly)
    TArray<FName> CluePropIds;

    UPROPERTY(Replicated, BlueprintReadOnly)
    TArray<int32> ClueNumbers;

    UPROPERTY(EditDefaultsOnly, Category = "Clue")
    int32 CluePropCount = 3;

    UPROPERTY(EditDefaultsOnly, Category = "Password")
    int32 PasswordCodeLength = 3;

    UPROPERTY(EditDefaultsOnly, Category = "Password")
    double RunnerCooldownWithOneClue = 30.0;

    UPROPERTY(EditDefaultsOnly, Category = "Password")
    double RunnerCooldownWithTwoClues = 20.0;

    UPROPERTY(EditDefaultsOnly, Category = "Password")
    double RunnerCooldownWithThreeClues = 10.0;

    UPROPERTY(EditDefaultsOnly, Category = "Password")
    double RunnerFailureCooldownStep = 10.0;

    UPROPERTY(EditDefaultsOnly, Category = "Password")
    double SeekerFailureCooldownStep = 10.0;

    UPROPERTY(Replicated)
    int32 RunnerCodeFailureCount = 0;

    UPROPERTY(Replicated)
    double RunnerCodeCooldownEndServerTime = 0.0;

    UPROPERTY(Replicated)
    int32 SeekerCodeFailureCount = 0;

    UPROPERTY(Replicated)
    double SeekerCodeCooldownEndServerTime = 0.0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game Phase")
    EGamePhase CurrentGamePhase = EGamePhase::None;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game Phase")
    double PhaseEndServerTime = 0.0;

    UPROPERTY(EditDefaultsOnly, Category = "Respawn")
    int32 MaxVisibleRespawnProps = 3;

    void SelectRandomClueProps();
    double GetRunnerClueBasedCooldown() const;
    double GetServerTimeSeconds() const;
    bool RefreshVisibleRespawnList();
    bool AppendVisibleRespawnEntry(int32 SourceIndex);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditDefaultsOnly, Category = "Clue")
    TSubclassOf<ACluePickup> CluePickupClass = nullptr;
};

