#include "MyGameState.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "CluePickup.h"
#include "MyGameMode.h"
#include "MyCharacter.h"
#include "MyPlayerController.h"
#include "MyPlayerState.h"
#include "PropBase.h"
#include "Components/StaticMeshComponent.h"

AMyGameState::AMyGameState()
{
    bReplicates = true;
}

void AMyGameState::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        SelectRandomClueProps();
    }
}

void AMyGameState::SelectRandomClueProps()
{
    CluePropIds.Empty();
    ClueNumbers.Empty();

    TArray<AActor*> FoundProps;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APropBase::StaticClass(), FoundProps);

    TArray<APropBase*> ValidProps;
    for (AActor* Actor : FoundProps)
    {
        if (APropBase* Prop = Cast<APropBase>(Actor))
        {
            ValidProps.Add(Prop);
        }
    }

    if (ValidProps.Num() <= 0)
    {
        return;
    }

    const int32 PickCount = FMath::Min(CluePropCount, ValidProps.Num());
    TArray<int32> AvailableClueNumbers;
    for (int32 Number = 0; Number <= 9; ++Number)
    {
        AvailableClueNumbers.Add(Number);
    }

    for (int32 i = 0; i < PickCount; ++i)
    {
        const int32 PropRandomIndex = FMath::RandRange(0, ValidProps.Num() - 1);
        APropBase* PickedProp = ValidProps[PropRandomIndex];
        ValidProps.RemoveAt(PropRandomIndex);

        if (PickedProp && AvailableClueNumbers.Num() > 0)
        {
            const int32 NumberRandomIndex = FMath::RandRange(0, AvailableClueNumbers.Num() - 1);
            const int32 PickedClueNumber = AvailableClueNumbers[NumberRandomIndex];
            AvailableClueNumbers.RemoveAt(NumberRandomIndex);

            CluePropIds.Add(PickedProp->GetPropId());
            ClueNumbers.Add(PickedClueNumber);
        }
    }

    ForceNetUpdate();
}

bool AMyGameState::DoesPropHaveClue(const FName& PropId) const
{
    return CluePropIds.Contains(PropId);
}

void AMyGameState::RegisterDestroyedProp(AActor* DestroyedProp)
{
    if (!HasAuthority() || !IsValid(DestroyedProp)) return;

    FRespawnPropInfo Info;
    Info.PropClass = DestroyedProp->GetClass();
    Info.Transform = DestroyedProp->GetActorTransform();
    if (const APropBase* DestroyedPropBase = Cast<APropBase>(DestroyedProp))
    {
        if (const UStaticMeshComponent* MeshComp = DestroyedPropBase->GetStaticMesh())
        {
            Info.Transform.SetScale3D(MeshComp->GetComponentScale());
        }
    }

    RespawnList.Add(Info);

    if (VisibleRespawnList.Num() < MaxVisibleRespawnProps)
    {
        AppendVisibleRespawnEntry(RespawnList.Num() - 1);
    }

    ForceNetUpdate();

}

bool AMyGameState::IsValidRespawnIndex(int32 Index) const
{
    return VisibleRespawnList.IsValidIndex(Index) && VisibleRespawnList[Index].PropClass != nullptr;
}

bool AMyGameState::ConsumeRespawnEntry(int32 Index, FRespawnPropInfo& OutInfo)
{
    if (!HasAuthority()) return false;
    if (!VisibleRespawnIndices.IsValidIndex(Index)) return false;

    const int32 SourceIndex = VisibleRespawnIndices[Index];
    if (!RespawnList.IsValidIndex(SourceIndex)) return false;
    if (!RespawnList[SourceIndex].PropClass) return false;

    OutInfo = RespawnList[SourceIndex];
    RespawnList.RemoveAt(SourceIndex);
    RefreshVisibleRespawnList();
    ForceNetUpdate();
    return true;
}

void AMyGameState::RestoreRespawnEntry(const FRespawnPropInfo& Info)
{
    if (!HasAuthority()) return;
    if (!Info.PropClass) return;

    RespawnList.Add(Info);
    RefreshVisibleRespawnList();
    ForceNetUpdate();
}

bool AMyGameState::RequestRefreshVisibleRespawnList()
{
    if (!HasAuthority())
    {
        return false;
    }

    if (!CanRefreshVisibleRespawnList())
    {
        return false;
    }

    const bool bRefreshed = RefreshVisibleRespawnList();
    if (bRefreshed)
    {
        ForceNetUpdate();
    }

    return bRefreshed;
}

bool AMyGameState::CanRefreshVisibleRespawnList() const
{
    int32 ValidRespawnCount = 0;
    for (const FRespawnPropInfo& Info : RespawnList)
    {
        if (Info.PropClass)
        {
            ++ValidRespawnCount;
        }
    }

    return ValidRespawnCount > MaxVisibleRespawnProps;
}

void AMyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMyGameState, RespawnList);
    DOREPLIFETIME(AMyGameState, VisibleRespawnList);
    DOREPLIFETIME(AMyGameState, VisibleRespawnIndices);
    DOREPLIFETIME(AMyGameState, CluePropIds);
    DOREPLIFETIME(AMyGameState, ClueNumbers);
    DOREPLIFETIME(AMyGameState, RunnerCodeFailureCount);
    DOREPLIFETIME(AMyGameState, RunnerCodeCooldownEndServerTime);
    DOREPLIFETIME(AMyGameState, SeekerCodeFailureCount);
    DOREPLIFETIME(AMyGameState, SeekerCodeCooldownEndServerTime);
    DOREPLIFETIME(AMyGameState, CurrentGamePhase);
    DOREPLIFETIME(AMyGameState, PhaseEndServerTime);
    DOREPLIFETIME(AMyGameState, GameResult);
}

void AMyGameState::BuildPositionedClueNumbers(
    const TArray<int32>& CollectedClueNumbers,
    TArray<int32>& OutPositionedClues) const
{
    OutPositionedClues.Init(INDEX_NONE, PasswordCodeLength);

    const int32 CodeLength = FMath::Min(PasswordCodeLength, ClueNumbers.Num());
    for (int32 Index = 0; Index < CodeLength; ++Index)
    {
        if (CollectedClueNumbers.Contains(ClueNumbers[Index]))
        {
            OutPositionedClues[Index] = ClueNumbers[Index];
        }
    }
}

void AMyGameState::SetGameResult_Server(EFinalRole WinningRole)
{
    if (!HasAuthority())
    {
        return;
    }

    GameResult.WinningRole = WinningRole;
    ++GameResult.Serial;
    ForceNetUpdate();
}

void AMyGameState::OnRep_GameResult()
{
    if (GameResult.Serial <= 0 || GameResult.WinningRole == EFinalRole::None || !GetWorld())
    {
        return;
    }

    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        PC->ShowGameResultLocal(GameResult.WinningRole);
    }
}

bool AMyGameState::AppendVisibleRespawnEntry(int32 SourceIndex)
{
    if (!RespawnList.IsValidIndex(SourceIndex))
    {
        return false;
    }

    if (!RespawnList[SourceIndex].PropClass)
    {
        return false;
    }

    if (VisibleRespawnIndices.Contains(SourceIndex))
    {
        return false;
    }

    VisibleRespawnIndices.Add(SourceIndex);
    VisibleRespawnList.Add(RespawnList[SourceIndex]);
    return true;
}

bool AMyGameState::RefreshVisibleRespawnList()
{
    if (!HasAuthority())
    {
        return false;
    }

    VisibleRespawnList.Empty();
    VisibleRespawnIndices.Empty();

    TArray<int32> CandidateIndices;
    CandidateIndices.Reserve(RespawnList.Num());

    for (int32 Index = 0; Index < RespawnList.Num(); ++Index)
    {
        if (RespawnList[Index].PropClass)
        {
            CandidateIndices.Add(Index);
        }
    }

    if (CandidateIndices.Num() <= 0)
    {
        return false;
    }

    const int32 PickCount = FMath::Min(MaxVisibleRespawnProps, CandidateIndices.Num());

    for (int32 i = 0; i < PickCount; ++i)
    {
        const int32 RandomPick = FMath::RandRange(0, CandidateIndices.Num() - 1);
        const int32 SourceIndex = CandidateIndices[RandomPick];
        CandidateIndices.RemoveAt(RandomPick);

        VisibleRespawnIndices.Add(SourceIndex);
        VisibleRespawnList.Add(RespawnList[SourceIndex]);
    }

    return VisibleRespawnList.Num() > 0;
}

bool AMyGameState::TrySpawnClueFromDestroyedProp(APropBase* DestroyedProp)
{
    if (!HasAuthority()) return false;
    if (!IsValid(DestroyedProp)) return false;
    if (!CluePickupClass) return false;

    const FName PropId = DestroyedProp->GetPropId();

    int32 ClueNumber = INDEX_NONE;
    if (!GetClueNumberByPropId(PropId, ClueNumber))
    {
        return false;
    }

    const FVector SpawnLoc = DestroyedProp->GetActorLocation() + FVector(0.f, 0.f, 80.f);
    const FRotator SpawnRot = FRotator::ZeroRotator;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    ACluePickup* Clue = GetWorld()->SpawnActor<ACluePickup>(
        CluePickupClass,
        SpawnLoc,
        SpawnRot,
        Params
    );

    if (!Clue)
    {
        return false;
    }

    Clue->SetClueNumber(ClueNumber);

    return true;
}

bool AMyGameState::GetClueNumberByPropId(const FName& PropId, int32& OutClueNumber) const
{
    const int32 FoundIndex = CluePropIds.IndexOfByKey(PropId);
    if (FoundIndex == INDEX_NONE)
    {
        return false;
    }

    if (!ClueNumbers.IsValidIndex(FoundIndex))
    {
        return false;
    }

    OutClueNumber = ClueNumbers[FoundIndex];
    return true;
}

double AMyGameState::GetServerTimeSeconds() const
{
    return GetServerWorldTimeSeconds();
}

double AMyGameState::GetRunnerCodeInputCooldownRemaining() const
{
    return GetCodeInputCooldownRemaining(EFinalRole::Runner);
}

double AMyGameState::GetCodeInputCooldownRemaining(EFinalRole FinalRole) const
{
    if (FinalRole == EFinalRole::Runner)
    {
        return FMath::Max(0.0, RunnerCodeCooldownEndServerTime - GetServerTimeSeconds());
    }

    if (FinalRole == EFinalRole::Seeker)
    {
        return FMath::Max(0.0, SeekerCodeCooldownEndServerTime - GetServerTimeSeconds());
    }

    return 0.0;
}

double AMyGameState::GetPhaseRemainingSeconds() const
{
    return FMath::Max(0.0, PhaseEndServerTime - GetServerTimeSeconds());
}

void AMyGameState::SetGamePhase_Server(EGamePhase NewPhase, double DurationSeconds)
{
    if (!HasAuthority())
    {
        return;
    }

    CurrentGamePhase = NewPhase;
    PhaseEndServerTime = DurationSeconds > 0.0 ? GetServerTimeSeconds() + DurationSeconds : 0.0;
    ForceNetUpdate();
}

int32 AMyGameState::CountRunnerTeamCollectedClues() const
{
    TSet<int32> UniqueClues;

    for (APlayerState* PS : PlayerArray)
    {
        const AMyPlayerState* MPS = Cast<AMyPlayerState>(PS);
        if (!MPS || MPS->GetFinalRole() != EFinalRole::Runner)
        {
            continue;
        }

        for (const int32 ClueNumber : MPS->GetCollectedClueNumbers())
        {
            UniqueClues.Add(ClueNumber);
        }
    }

    return UniqueClues.Num();
}

void AMyGameState::GetActiveTeamCounts(int32& OutSeekerCount, int32& OutRunnerCount) const
{
    OutSeekerCount = 0;
    OutRunnerCount = 0;

    for (APlayerState* PS : PlayerArray)
    {
        const AMyPlayerState* MPS = Cast<AMyPlayerState>(PS);
        if (!MPS)
        {
            continue;
        }

        const AMyCharacter* Character = Cast<AMyCharacter>(MPS->GetPawn());
        if (!Character || Character->IsEliminated())
        {
            continue;
        }

        if (MPS->GetFinalRole() == EFinalRole::Seeker)
        {
            ++OutSeekerCount;
        }
        else if (MPS->GetFinalRole() == EFinalRole::Runner)
        {
            ++OutRunnerCount;
        }
    }
}

double AMyGameState::GetRunnerClueBasedCooldown() const
{
    const int32 ClueCount = CountRunnerTeamCollectedClues();
    if (ClueCount >= 3)
    {
        return RunnerCooldownWithThreeClues;
    }

    if (ClueCount == 2)
    {
        return RunnerCooldownWithTwoClues;
    }

    return RunnerCooldownWithOneClue;
}

bool AMyGameState::SubmitPasswordCode(APlayerController* RequestPC, const TArray<int32>& InputCode, int32& OutCorrectCount, double& OutCooldownRemaining)
{
    OutCorrectCount = 0;
    OutCooldownRemaining = 0.0;

    if (!HasAuthority() || !RequestPC)
    {
        return false;
    }

    const AMyPlayerState* RequestPS = RequestPC->GetPlayerState<AMyPlayerState>();
    if (!RequestPS)
    {
        return false;
    }

    const EFinalRole RequestRole = RequestPS->GetFinalRole();
    if (RequestRole != EFinalRole::Runner && RequestRole != EFinalRole::Seeker)
    {
        return false;
    }

    OutCooldownRemaining = GetCodeInputCooldownRemaining(RequestRole);
    if (OutCooldownRemaining > 0.0)
    {
        return false;
    }

    TSet<int32> UniqueInputDigits;
    for (const int32 Digit : InputCode)
    {
        if (Digit < 0 || Digit > 9 || UniqueInputDigits.Contains(Digit))
        {
            return false;
        }

        UniqueInputDigits.Add(Digit);
    }

    if (InputCode.Num() != PasswordCodeLength)
    {
        return false;
    }

    const int32 CompareLength = FMath::Min3(PasswordCodeLength, InputCode.Num(), ClueNumbers.Num());
    for (int32 Index = 0; Index < CompareLength; ++Index)
    {
        if (InputCode[Index] == ClueNumbers[Index])
        {
            ++OutCorrectCount;
        }
    }

    const bool bSuccess = InputCode.Num() == PasswordCodeLength
        && ClueNumbers.Num() >= PasswordCodeLength
        && OutCorrectCount == PasswordCodeLength;

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Password victory: role=%d, correct=%d"), static_cast<int32>(RequestRole), OutCorrectCount);

        if (AMyGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMyGameMode>() : nullptr)
        {
            GM->HandleCodeVictory(RequestRole);
        }

        return true;
    }

    if (RequestRole == EFinalRole::Runner)
    {
        ++RunnerCodeFailureCount;
        const double FailureCooldown = RunnerCodeFailureCount * RunnerFailureCooldownStep;
        const double Cooldown = FMath::Max(GetRunnerClueBasedCooldown(), FailureCooldown);
        RunnerCodeCooldownEndServerTime = GetServerTimeSeconds() + Cooldown;
        OutCooldownRemaining = Cooldown;
        ForceNetUpdate();
    }
    else if (RequestRole == EFinalRole::Seeker)
    {
        ++SeekerCodeFailureCount;
        const double Cooldown = SeekerCodeFailureCount * SeekerFailureCooldownStep;
        SeekerCodeCooldownEndServerTime = GetServerTimeSeconds() + Cooldown;
        OutCooldownRemaining = Cooldown;
        ForceNetUpdate();
    }

    return false;
}

