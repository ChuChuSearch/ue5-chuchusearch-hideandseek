#include "MyGameMode.h"
#include "MyCharacter.h"
#include "GhostCharacter.h"
#include "MyPlayerController.h"
#include "MyGameState.h"
#include "PropBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"

#include "MyPlayerState.h"

AMyGameMode::AMyGameMode()
{
    PlayerStateClass = AMyPlayerState::StaticClass();
    DefaultPawnClass = nullptr;
    bUseSeamlessTravel = true;
}

void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority() && bAutoStartGameTimeline)
    {
        StartGameTimeline();
    }
}


void AMyGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    Super::HandleStartingNewPlayer_Implementation(NewPlayer);

    if (!HasAuthority() || !IsValid(NewPlayer)) return;
    if (!bAutoSpawnOnLogin) return;

    if (NewPlayer->GetPawn())
    {
        return;
    }

    AMyPlayerState* PS = NewPlayer->GetPlayerState<AMyPlayerState>();
    const bool bSeeker = (PS && PS->GetFinalRole() == EFinalRole::Seeker);

    SpawnPlayerAsRole(NewPlayer, bSeeker);
}

void AMyGameMode::SpawnPlayerAsRole(APlayerController* PC, bool bSeeker)
{
    if (!HasAuthority() || !IsValid(PC)) return;
    if (!PlayerCharacterClass) return;

    AActor* Start = FindPlayerStart(PC);
    if (!Start) return;

    const FTransform SpawnTM = Start->GetActorTransform();

    FActorSpawnParameters Params;
    Params.Owner = PC;
    Params.Instigator = nullptr;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AMyCharacter* NewChar = GetWorld()->SpawnActor<AMyCharacter>(PlayerCharacterClass, SpawnTM, Params);
    if (!NewChar) return;

    PC->Possess(NewChar);
    NewChar->SetIsSeeker_Server(bSeeker);
}

void AMyGameMode::PostSeamlessTravel()
{
    Super::PostSeamlessTravel();

    // ���⼭ Pawn ����/���� ������ ���� �� ��
}

void AMyGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
    Super::HandleSeamlessTravelPlayer(C);

    APlayerController* PC = Cast<APlayerController>(C);
    if (!HasAuthority() || !IsValid(PC)) return;

    AMyPlayerState* PS = PC->GetPlayerState<AMyPlayerState>();
    if (!PS) return;

    const bool bSeeker = (PS->GetFinalRole() == EFinalRole::Seeker);

    if (AMyCharacter* ExistingChar = Cast<AMyCharacter>(PC->GetPawn()))
    {
        ExistingChar->SetIsSeeker_Server(bSeeker);
        return;
    }

    SpawnPlayerAsRole(PC, bSeeker);
}

void AMyGameMode::HandleRunnerEliminated(AMyCharacter* EliminatedRunner)
{
    if (!HasAuthority() || !IsValid(EliminatedRunner)) return;

    AController* PC = EliminatedRunner->GetController();
    if (!PC) return;

    const FVector SpawnLoc = EliminatedRunner->GetActorLocation();
    const FRotator SpawnRot = EliminatedRunner->GetActorRotation();

    if (EliminatedRunner->IsPossessing())
    {
        EliminatedRunner->ForceReleasePossessedProp_Server();
    }

    EliminatedRunner->ApplyEliminatedVisuals();

    if (!GhostCharacterClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleRunnerEliminated: GhostCharacterClass is not set."));
        if (!HasAnyActiveRunner())
        {
            EndGameWithWinner(EFinalRole::Seeker);
        }
        return;
    }

    FActorSpawnParameters Params;
    Params.Owner = PC;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AGhostCharacter* Ghost = GetWorld()->SpawnActor<AGhostCharacter>(
        GhostCharacterClass,
        SpawnLoc,
        SpawnRot,
        Params
    );

    if (!Ghost)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleRunnerEliminated: Failed to spawn ghost character."));
        if (!HasAnyActiveRunner())
        {
            EndGameWithWinner(EFinalRole::Seeker);
        }
        return;
    }

    PC->Possess(Ghost);

    EliminatedRunner->Destroy();

    if (AMyPlayerController* MPC = Cast<AMyPlayerController>(PC))
    {
        MPC->ClientFinalizeGhostTransition();
    }

    if (!HasAnyActiveRunner())
    {
        EndGameWithWinner(EFinalRole::Seeker);
    }
}

void AMyGameMode::HandleCodeVictory(EFinalRole WinningRole)
{
    EndGameWithWinner(WinningRole);
}

void AMyGameMode::EndGameWithWinner(EFinalRole WinningRole)
{
    if (!HasAuthority() || bGameEnded)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("EndGameWithWinner: WinningRole=%d"), static_cast<int32>(WinningRole));

    bGameEnded = true;

    GetWorldTimerManager().ClearTimer(TH_GamePhase);

    if (AMyGameState* GS = GetGameState<AMyGameState>())
    {
        GS->SetGamePhase_Server(EGamePhase::Ended, 0.0);
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AMyPlayerController* MPC = Cast<AMyPlayerController>(It->Get()))
        {
            MPC->ClientShowGameResult(WinningRole);
        }
    }

    OnCodeVictory(WinningRole);
    OnGamePhaseChanged(EGamePhase::Ended);
}

void AMyGameMode::NotifyRunnerReachedSpecialExit(AMyCharacter* Runner)
{
    if (!HasAuthority() || bGameEnded || !IsValid(Runner))
    {
        return;
    }

    if (Runner->IsSeeker() || Runner->IsEliminated())
    {
        return;
    }

    const AMyGameState* GS = GetGameState<AMyGameState>();
    if (!GS || GS->CountRunnerTeamCollectedClues() < RequiredCluesForRunnerExit)
    {
        return;
    }

    EndGameWithWinner(EFinalRole::Runner);
}

void AMyGameMode::StartGameTimeline()
{
    if (!HasAuthority() || bGameEnded)
    {
        return;
    }

    EnterGamePhase(EGamePhase::HideTime, HideTimeSeconds);
    GetWorldTimerManager().SetTimer(TH_GamePhase, this, &AMyGameMode::EnterPhase1, HideTimeSeconds, false);
}

void AMyGameMode::EnterGamePhase(EGamePhase NewPhase, double DurationSeconds)
{
    if (!HasAuthority() || bGameEnded)
    {
        return;
    }

    if (AMyGameState* GS = GetGameState<AMyGameState>())
    {
        GS->SetGamePhase_Server(NewPhase, DurationSeconds);
    }

    OnGamePhaseChanged(NewPhase);
}

void AMyGameMode::EnterPhase1()
{
    EnterGamePhase(EGamePhase::Phase1, Phase1Seconds);
    GetWorldTimerManager().SetTimer(TH_GamePhase, this, &AMyGameMode::EnterForcedSwap, Phase1Seconds, false);
}

void AMyGameMode::EnterForcedSwap()
{
    EnterGamePhase(EGamePhase::ForcedSwap, ForcedSwapSeconds);
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        AMyCharacter* Character = PC ? Cast<AMyCharacter>(PC->GetPawn()) : nullptr;
        if (!Character || !IsPhase1PossessedProp(Character->GetPossessedProp()))
        {
            continue;
        }

        if (AMyPlayerController* MPC = Cast<AMyPlayerController>(PC))
        {
            MPC->ClientSetForcedSwapWarning(true, ForcedSwapSeconds);
        }
    }
    GetWorldTimerManager().SetTimer(TH_GamePhase, this, &AMyGameMode::EnterPhase2, ForcedSwapSeconds, false);
}

void AMyGameMode::EnterPhase2()
{
    ResolveForcedSwapProps();
    EnterGamePhase(EGamePhase::Phase2, Phase2Seconds);
    GetWorldTimerManager().SetTimer(TH_GamePhase, this, &AMyGameMode::EnterFeverTime, Phase2Seconds, false);
}

void AMyGameMode::EnterFeverTime()
{
    ClearPhase1PropBans();
    EnterGamePhase(EGamePhase::FeverTime, FeverTimeSeconds);
    GetWorldTimerManager().SetTimer(TH_GamePhase, this, &AMyGameMode::EndGameByTime, FeverTimeSeconds, false);
}

void AMyGameMode::EndGameByTime()
{
    EndGameWithWinner(EFinalRole::Runner);
}

void AMyGameMode::RegisterPhase1PossessedProp(APropBase* Prop)
{
    if (!HasAuthority() || !IsValid(Prop))
    {
        return;
    }

    AMyGameState* GS = GetGameState<AMyGameState>();
    if (!GS)
    {
        return;
    }

    const EGamePhase Phase = GS->GetGamePhase();
    if (Phase != EGamePhase::HideTime && Phase != EGamePhase::Phase1)
    {
        return;
    }

    Phase1PossessedProps.Add(Prop);
}

bool AMyGameMode::IsPhase1PossessedProp(const APropBase* Prop) const
{
    if (!Prop)
    {
        return false;
    }

    return Phase1PossessedProps.Contains(const_cast<APropBase*>(Prop));
}

void AMyGameMode::ClearPhase1PropBans()
{
    if (!HasAuthority())
    {
        BannedPhase1Props.Empty();
        return;
    }

    for (const TWeakObjectPtr<APropBase>& PropPtr : BannedPhase1Props)
    {
        if (APropBase* Prop = PropPtr.Get())
        {
            Prop->SetPossessionBanned_Server(false);
        }
    }

    BannedPhase1Props.Empty();
}

bool AMyGameMode::HasAnyActiveRunner() const
{
    if (!GetWorld())
    {
        return false;
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        const AMyPlayerState* PS = PC ? PC->GetPlayerState<AMyPlayerState>() : nullptr;
        if (!PS || PS->GetFinalRole() != EFinalRole::Runner)
        {
            continue;
        }

        const AMyCharacter* Character = Cast<AMyCharacter>(PC->GetPawn());
        if (Character && !Character->IsEliminated())
        {
            return true;
        }
    }

    return false;
}

void AMyGameMode::ResolveForcedSwapProps()
{
    if (!HasAuthority())
    {
        Phase1PossessedProps.Empty();
        return;
    }

    for (const TWeakObjectPtr<APropBase>& PropPtr : Phase1PossessedProps)
    {
        if (APropBase* Prop = PropPtr.Get())
        {
            Prop->SetPossessionBanned_Server(true);
            BannedPhase1Props.Add(Prop);
        }
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC)
        {
            continue;
        }

        const AMyPlayerState* PS = PC->GetPlayerState<AMyPlayerState>();
        if (!PS || PS->GetFinalRole() != EFinalRole::Runner)
        {
            continue;
        }

        AMyCharacter* Character = Cast<AMyCharacter>(PC->GetPawn());
        if (!Character)
        {
            continue;
        }

        if (AMyPlayerController* MPC = Cast<AMyPlayerController>(PC))
        {
            MPC->ClientSetForcedSwapWarning(false, 0.0);
        }

        APropBase* CurrentProp = Character->GetPossessedProp();
        if (!CurrentProp || !IsPhase1PossessedProp(CurrentProp))
        {
            continue;
        }

        if (ForcedSwapPropClasses.Num() <= 0)
        {
            Character->RequestReleaseProp();
            continue;
        }

        const int32 PickIndex = FMath::RandRange(0, ForcedSwapPropClasses.Num() - 1);
        Character->ForceReplacePossessedProp_Server(ForcedSwapPropClasses[PickIndex]);
    }

    Phase1PossessedProps.Empty();
}
