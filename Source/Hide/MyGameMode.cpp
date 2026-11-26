#include "MyGameMode.h"
#include "MyCharacter.h"
#include "GhostCharacter.h"
#include "MyPlayerController.h"
#include "MyGameState.h"
#include "PropBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Engine/NetConnection.h"

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


bool AMyGameMode::IsPlayInEditorWorld() const
{
    const UWorld* World = GetWorld();
    return World && World->WorldType == EWorldType::PIE;
}

void AMyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (!ErrorMessage.IsEmpty())
    {
        return;
    }

    // seamless travel로 넘어온 플레이어는 PreLogin을 거치지 않으므로,
    // 여기 들어오는 건 게임 도중 새로 접속하려는 경우다.
    if (!bRejectJoinInProgress || IsPlayInEditorWorld())
    {
        return;
    }

    // 이번 경기 참가자가 끊겼다가 다시 들어오는 경우는 허용
    if (bAllowReconnect && !bGameEnded && DisconnectedPlayers.Contains(MakeReconnectKey(UniqueId, Address)))
    {
        UE_LOG(LogTemp, Log, TEXT("Accepting reconnect login from %s."), *Address);
        return;
    }

    ErrorMessage = TEXT("Match in progress");
    UE_LOG(LogTemp, Log, TEXT("Rejected join-in-progress login from %s."), *Address);
}

void AMyGameMode::PostLogin(APlayerController* NewPlayer)
{
    // Super 안에서 HandleStartingNewPlayer가 호출되는데, 재접속자는 거기서 새 캐릭터를 만들지 않는다.
    Super::PostLogin(NewPlayer);

    TryRestoreReconnectingPlayer(NewPlayer);
}

FString AMyGameMode::MakeReconnectKey(const FUniqueNetIdRepl& UniqueId, const FString& Address)
{
    // 온라인 ID 우선(Null 서브시스템: PC별 고정 ID, Steam: SteamID). 없으면 IP로 대체.
    if (UniqueId.IsValid())
    {
        return FString::Printf(TEXT("ID:%s"), *UniqueId.ToString());
    }

    return FString::Printf(TEXT("IP:%s"), *Address);
}

FString AMyGameMode::GetReconnectKey(const APlayerController* PC) const
{
    if (!PC)
    {
        return FString();
    }

    const APlayerState* PS = PC->GetPlayerState<APlayerState>();
    const FUniqueNetIdRepl UniqueId = PS ? PS->GetUniqueId() : FUniqueNetIdRepl();

    FString Address;
    if (UNetConnection* Conn = PC->GetNetConnection())
    {
        Address = Conn->LowLevelGetRemoteAddress(false);
    }

    return MakeReconnectKey(UniqueId, Address);
}

bool AMyGameMode::IsDisconnectedPlayerPawn(const APawn* Pawn) const
{
    if (!Pawn)
    {
        return false;
    }

    for (const TPair<FString, FDisconnectedPlayerRecord>& Pair : DisconnectedPlayers)
    {
        if (Pair.Value.Pawn.Get() == Pawn)
        {
            return true;
        }
    }

    return false;
}

bool AMyGameMode::TryRestoreReconnectingPlayer(APlayerController* PC)
{
    if (!HasAuthority() || !IsValid(PC) || !bAllowReconnect || bGameEnded)
    {
        return false;
    }

    FDisconnectedPlayerRecord Record;
    if (!DisconnectedPlayers.RemoveAndCopyValue(GetReconnectKey(PC), Record))
    {
        return false;
    }

    AMyPlayerState* PS = PC->GetPlayerState<AMyPlayerState>();
    if (!PS)
    {
        return false;
    }

    PS->ImportReconnectData_Server(Record.Data);
    const bool bSeeker = Record.Data.FinalRole == EFinalRole::Seeker;

    UE_LOG(LogTemp, Log, TEXT("Player reconnected: %s (role=%d)"), *PS->GetNickname(), static_cast<int32>(Record.Data.FinalRole));

    APawn* SavedPawn = Record.Pawn.Get();
    if (IsValid(SavedPawn) && !SavedPawn->GetController())
    {
        PC->Possess(SavedPawn);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Reconnect: saved pawn missing, spawning new one for %s."), *PS->GetNickname());
        SpawnPlayerAsRole(PC, bSeeker);
    }

    // 끊긴 사이에 탈락한 러너는 다시 들어오면 유령으로 전환
    if (AMyCharacter* MC = Cast<AMyCharacter>(PC->GetPawn()))
    {
        if (MC->IsEliminated() && !MC->IsSeeker())
        {
            HandleRunnerEliminated(MC);
            return true;
        }
    }

    if (AMyPlayerController* MPC = Cast<AMyPlayerController>(PC))
    {
        if (PC->GetPawn() && PC->GetPawn()->IsA<AGhostCharacter>())
        {
            MPC->ClientFinalizeGhostTransition();
        }
        else if (const AMyCharacter* MC = Cast<AMyCharacter>(PC->GetPawn()))
        {
            MPC->ClientRestoreAfterReconnect(MC->GetSeekerHitCount());
        }
    }

    return true;
}

void AMyGameMode::OnReconnectGraceExpired()
{
    EvaluateTeamsAfterLogout(nullptr, false);
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

    // 재접속자는 PostLogin에서 기존 캐릭터를 다시 조종하게 하므로 새로 만들지 않는다.
    if (DisconnectedPlayers.Contains(GetReconnectKey(NewPlayer)))
    {
        return;
    }

    AMyPlayerState* PS = NewPlayer->GetPlayerState<AMyPlayerState>();

    // 대기실에서 역할을 받지 못한 플레이어에게는 캐릭터를 주지 않는다(PIE 테스트 제외).
    if ((!PS || PS->GetFinalRole() == EFinalRole::None) && !IsPlayInEditorWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("Not spawning %s: no assigned role."), *GetNameSafe(NewPlayer));
        return;
    }

    const bool bSeeker = (PS && PS->GetFinalRole() == EFinalRole::Seeker);

    SpawnPlayerAsRole(NewPlayer, bSeeker);
}

void AMyGameMode::Logout(AController* Exiting)
{
    const AMyPlayerState* ExitingPlayerState = Exiting
        ? Exiting->GetPlayerState<AMyPlayerState>()
        : nullptr;

    // 경기 중 원격 플레이어가 끊기면 상태와 캐릭터를 보관해 두고 재접속을 기다린다.
    // (캐릭터는 AMyPlayerController::PawnLeavingGame에서 파괴되지 않고 남는다)
    APlayerController* ExitingPC = Cast<APlayerController>(Exiting);
    if (ShouldKeepPawnOnDisconnect()
        && ExitingPC
        && !ExitingPC->IsLocalController()
        && ExitingPlayerState
        && ExitingPlayerState->GetFinalRole() != EFinalRole::None)
    {
        const FString Key = GetReconnectKey(ExitingPC);
        if (!Key.IsEmpty())
        {
            FDisconnectedPlayerRecord Record;
            ExitingPlayerState->ExportReconnectData(Record.Data);
            Record.Pawn = ExitingPC->GetPawn();
            DisconnectedPlayers.Add(Key, Record);

            UE_LOG(LogTemp, Log, TEXT("Player disconnected mid-match, waiting for reconnect: %s"), *ExitingPlayerState->GetNickname());
        }
    }

    Super::Logout(Exiting);

    EvaluateTeamsAfterLogout(ExitingPlayerState);
}

void AMyGameMode::EvaluateTeamsAfterLogout(const AMyPlayerState* ExitingPlayerState, bool bAllowGrace)
{
    if (!HasAuthority() || !GameState)
    {
        return;
    }

    int32 RemainingPlayers = 0;
    int32 RemainingSeekers = 0;
    int32 RemainingRunners = 0;

    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        const AMyPlayerState* MyPlayerState = Cast<AMyPlayerState>(PlayerState);
        if (!MyPlayerState || MyPlayerState == ExitingPlayerState)
        {
            continue;
        }

        ++RemainingPlayers;

        if (MyPlayerState->GetFinalRole() == EFinalRole::Seeker)
        {
            ++RemainingSeekers;
        }
        else if (MyPlayerState->GetFinalRole() == EFinalRole::Runner)
        {
            ++RemainingRunners;
        }
    }

    if (RemainingPlayers == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Match abandoned: all players disconnected. Returning server to wait room."));

        bGameEnded = true;
        GetWorldTimerManager().ClearTimer(TH_GamePhase);
        GetWorldTimerManager().ClearTimer(TH_ReturnToWaitRoom);

        if (AMyGameState* GS = GetGameState<AMyGameState>())
        {
            GS->SetGamePhase_Server(EGamePhase::Ended, 0.0);
        }

        ReturnToWaitRoom();
        return;
    }

    if (bGameEnded)
    {
        return;
    }

    const bool bTeamMissing = (RemainingSeekers == 0 && RemainingRunners > 0)
        || (RemainingRunners == 0 && RemainingSeekers > 0);

    if (!bTeamMissing)
    {
        return;
    }

    // 한 팀이 전원 끊겼으면 바로 승패를 내지 않고 재접속을 잠시 기다린다.
    if (bAllowGrace && bAllowReconnect && ReconnectGraceSeconds > 0.f)
    {
        if (!GetWorldTimerManager().IsTimerActive(TH_ReconnectGrace))
        {
            UE_LOG(LogTemp, Log, TEXT("A team has no connected players. Waiting %.0fs for reconnect."), ReconnectGraceSeconds);
            GetWorldTimerManager().SetTimer(
                TH_ReconnectGrace,
                this,
                &AMyGameMode::OnReconnectGraceExpired,
                ReconnectGraceSeconds,
                false);
        }
        return;
    }

    if (RemainingSeekers == 0 && RemainingRunners > 0)
    {
        EndGameWithWinner(EFinalRole::Runner);
    }
    else if (RemainingRunners == 0 && RemainingSeekers > 0)
    {
        EndGameWithWinner(EFinalRole::Seeker);
    }
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
    if (!PC)
    {
        // 접속이 끊긴 러너가 탈락한 경우: 캐릭터는 남겨두고(재접속 시 유령으로 전환) 승패만 확인
        if (EliminatedRunner->IsPossessing())
        {
            EliminatedRunner->ForceReleasePossessedProp_Server();
        }

        EliminatedRunner->ApplyEliminatedVisuals();

        if (!HasAnyActiveRunner())
        {
            EndGameWithWinnerDelayed(EFinalRole::Seeker, EliminationResultDelaySeconds);
        }
        return;
    }

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
            EndGameWithWinnerDelayed(EFinalRole::Seeker, EliminationResultDelaySeconds);
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
            EndGameWithWinnerDelayed(EFinalRole::Seeker, EliminationResultDelaySeconds);
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
        EndGameWithWinnerDelayed(EFinalRole::Seeker, EliminationResultDelaySeconds);
    }
}

void AMyGameMode::HandleCodeVictory(EFinalRole WinningRole)
{
    EndGameWithWinner(WinningRole);
}

void AMyGameMode::EndGameWithWinnerDelayed(EFinalRole WinningRole, float DelaySeconds)
{
    if (!HasAuthority() || bGameEnded || bResultPending)
    {
        return;
    }

    if (DelaySeconds <= 0.f)
    {
        EndGameWithWinner(WinningRole);
        return;
    }

    // 승패는 지금 확정: 페이즈 타이머를 멈춰 대기 중에 시간 만료로 결과가 바뀌지 않게 한다
    bResultPending = true;
    PendingWinner = WinningRole;
    GetWorldTimerManager().ClearTimer(TH_GamePhase);
    GetWorldTimerManager().SetTimer(TH_PendingResult, this, &AMyGameMode::FinishPendingResult, DelaySeconds, false);
}

void AMyGameMode::FinishPendingResult()
{
    bResultPending = false;
    EndGameWithWinner(PendingWinner);
}

void AMyGameMode::EndGameWithWinner(EFinalRole WinningRole)
{
    if (!HasAuthority() || bGameEnded || bResultPending)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("EndGameWithWinner: WinningRole=%d"), static_cast<int32>(WinningRole));

    bGameEnded = true;

    GetWorldTimerManager().ClearTimer(TH_GamePhase);

    if (AMyGameState* GS = GetGameState<AMyGameState>())
    {
        GS->SetGamePhase_Server(EGamePhase::Ended, 0.0);
        GS->SetGameResult_Server(WinningRole);
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

    if (bAutoReturnToWaitRoom)
    {
        if (ReturnToWaitRoomDelaySeconds <= 0.f)
        {
            ReturnToWaitRoom();
        }
        else
        {
            GetWorldTimerManager().SetTimer(
                TH_ReturnToWaitRoom,
                this,
                &AMyGameMode::ReturnToWaitRoom,
                ReturnToWaitRoomDelaySeconds,
                false
            );
        }
    }
}

void AMyGameMode::ReturnToWaitRoom()
{
    if (!HasAuthority() || !GetWorld() || WaitRoomTravelURL.IsEmpty() || bReturningToWaitRoom)
    {
        return;
    }

    bReturningToWaitRoom = true;
    GetWorldTimerManager().ClearTimer(TH_ReturnToWaitRoom);

    UE_LOG(LogTemp, Log, TEXT("Returning all players to wait room: %s"), *WaitRoomTravelURL);

    // Returning to the lobby should discard match-only controllers, pawns, and UI.
    // A seamless travel can preserve the result widget and leave clients on its black frame.
    bUseSeamlessTravel = false;
    GetWorld()->ServerTravel(WaitRoomTravelURL, true);
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

int32 AMyGameMode::CountActiveRunners() const
{
    int32 Count = 0;

    if (!GetWorld())
    {
        return 0;
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
            ++Count;
        }
    }

    // 접속이 끊겨 재접속을 기다리는 러너의 캐릭터도 살아 있으면 활성 러너로 본다.
    for (const TPair<FString, FDisconnectedPlayerRecord>& Pair : DisconnectedPlayers)
    {
        if (Pair.Value.Data.FinalRole != EFinalRole::Runner)
        {
            continue;
        }

        const AMyCharacter* Character = Cast<AMyCharacter>(Pair.Value.Pawn.Get());
        if (Character && !Character->IsEliminated())
        {
            ++Count;
        }
    }

    return Count;
}

bool AMyGameMode::HasAnyActiveRunner() const
{
    return CountActiveRunners() > 0;
}

void AMyGameMode::BroadcastEliminationNotice(const AMyCharacter* EliminatedRunner) const
{
    if (!GetWorld() || !IsValid(EliminatedRunner))
    {
        return;
    }

    FString Name;
    if (const AMyPlayerState* PS = EliminatedRunner->GetPlayerState<AMyPlayerState>())
    {
        Name = PS->GetNickname().IsEmpty() ? PS->GetPlayerName() : PS->GetNickname();
    }

    UE_LOG(LogTemp, Log, TEXT("EliminationNotice: name='%s' (PS=%s)"), *Name, *GetNameSafe(EliminatedRunner->GetPlayerState()));

    // 탈락 처리(bEliminated=true) 뒤에 호출되므로 남은 수에서 이미 빠져 있다
    const int32 Remaining = CountActiveRunners();

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AMyPlayerController* MPC = Cast<AMyPlayerController>(It->Get()))
        {
            MPC->ClientShowEliminationNotice(Name, Remaining);
        }
    }
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
