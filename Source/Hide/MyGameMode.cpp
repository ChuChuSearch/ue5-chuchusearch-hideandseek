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
#include "EngineUtils.h"
#include "LobbyGameSettings.h"
#include "Engine/GameInstance.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#include "MyPlayerState.h"

AMyGameMode::AMyGameMode()
{
    PlayerStateClass = AMyPlayerState::StaticClass();
    DefaultPawnClass = nullptr;
    bUseSeamlessTravel = true;

    static ConstructorHelpers::FObjectFinder<USoundBase> StallSound(TEXT("/Game/Game/Sounds/Phase/SFX_Phase_Whistle.SFX_Phase_Whistle"));
    if (StallSound.Succeeded())
    {
        StallTauntSound = StallSound.Object;
    }
}

UClass* AMyGameMode::GetPlayerCharacterClass() const
{
    return PlayerCharacterClass.Get();
}

void AMyGameMode::FillPhaseSettings(FLobbyGameSettings& OutSettings) const
{
    OutSettings.HideTimeSeconds = FMath::RoundToInt(HideTimeSeconds);
    OutSettings.Phase1Seconds = FMath::RoundToInt(Phase1Seconds);
    OutSettings.ForcedSwapSeconds = FMath::RoundToInt(ForcedSwapSeconds);
    OutSettings.Phase2Seconds = FMath::RoundToInt(Phase2Seconds);
    OutSettings.FeverTimeSeconds = FMath::RoundToInt(FeverTimeSeconds);
}

void AMyGameMode::BeginPlay()
{
    // 방장이 대기실에서 정한 단계 시간으로 시작한다(대기실을 거치지 않고 바로 이 맵을 열면 블루프린트 값 그대로)
    if (const ULobbySettingsSubsystem* Sub = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULobbySettingsSubsystem>() : nullptr)
    {
        if (Sub->IsInitialized())
        {
            const FLobbyGameSettings& S = Sub->GetSettings();
            HideTimeSeconds = S.HideTimeSeconds;
            Phase1Seconds = S.Phase1Seconds;
            ForcedSwapSeconds = S.ForcedSwapSeconds;
            Phase2Seconds = S.Phase2Seconds;
            FeverTimeSeconds = S.FeverTimeSeconds;
            UE_LOG(LogTemp, Log, TEXT("Game timeline from lobby settings: hide=%d p1=%d swap=%d p2=%d fever=%d"),
                S.HideTimeSeconds, S.Phase1Seconds, S.ForcedSwapSeconds, S.Phase2Seconds, S.FeverTimeSeconds);
        }
    }

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

    const FString Key = GetReconnectKey(PC);
    FDisconnectedPlayerRecord Record;
    if (!DisconnectedPlayers.RemoveAndCopyValue(Key, Record))
    {
        return false;
    }

    // 돌아왔으니 개인 재접속 시간은 멈춘다
    if (FTimerHandle* Handle = PlayerReconnectTimers.Find(Key))
    {
        GetWorldTimerManager().ClearTimer(*Handle);
        PlayerReconnectTimers.Remove(Key);
    }

    AMyPlayerState* PS = PC->GetPlayerState<AMyPlayerState>();
    if (!PS)
    {
        return false;
    }

    PS->ImportReconnectData_Server(Record.Data);
    const bool bSeeker = Record.Data.FinalRole == EFinalRole::Seeker;

    UE_LOG(LogTemp, Log, TEXT("Player reconnected: %s (role=%d)"), *PS->GetNickname(), static_cast<int32>(Record.Data.FinalRole));

    BroadcastConnectionNotice(PS, EConnectionNoticeKind::Reconnected, FString());

    // 비어 있던 팀이 다시 채워졌으면 팀 전원 이탈 대기를 멈춘다
    if (GetWorldTimerManager().IsTimerActive(TH_ReconnectGrace) && !IsAnyTeamMissing())
    {
        GetWorldTimerManager().ClearTimer(TH_ReconnectGrace);
        if (AMyGameState* GS = GetGameState<AMyGameState>())
        {
            GS->SetReconnectWait_Server(EFinalRole::None, 0.0);
        }
    }

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
    if (AMyGameState* GS = GetGameState<AMyGameState>())
    {
        GS->SetReconnectWait_Server(EFinalRole::None, 0.0);
    }
    EvaluateTeamsAfterLogout(nullptr, false);
}

bool AMyGameMode::IsAnyTeamMissing() const
{
    if (!GameState)
    {
        return false;
    }

    int32 Seekers = 0;
    int32 Runners = 0;
    for (APlayerState* PS : GameState->PlayerArray)
    {
        if (const AMyPlayerState* MPS = Cast<AMyPlayerState>(PS))
        {
            Seekers += MPS->GetFinalRole() == EFinalRole::Seeker ? 1 : 0;
            Runners += MPS->GetFinalRole() == EFinalRole::Runner ? 1 : 0;
        }
    }

    return (Seekers == 0 && Runners > 0) || (Runners == 0 && Seekers > 0);
}

void AMyGameMode::BroadcastConnectionNotice(const AMyPlayerState* About, EConnectionNoticeKind Kind, const FString& SubText) const
{
    if (!GetWorld() || !About)
    {
        return;
    }

    const FString Name = About->GetNickname().IsEmpty() ? About->GetPlayerName() : About->GetNickname();
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMyPlayerController* MPC = Cast<AMyPlayerController>(It->Get());
        // 본인에게는 보내지 않는다(끊긴 사람은 어차피 못 받고, 돌아온 사람은 자기 복귀 안내가 필요 없다)
        if (MPC && MPC->PlayerState != About)
        {
            MPC->ClientShowConnectionNotice(Name, Kind, SubText);
        }
    }
}

void AMyGameMode::OnPlayerReconnectExpired(FString Key)
{
    PlayerReconnectTimers.Remove(Key);

    FDisconnectedPlayerRecord Record;
    if (!HasAuthority() || bGameEnded || !DisconnectedPlayers.RemoveAndCopyValue(Key, Record))
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Reconnect time expired: %s"), *Record.Data.Nickname);

    // 남아 있던 캐릭터 정리(숨어 있던 물체에서 빼낸 뒤 없앤다)
    APawn* Pawn = Record.Pawn.Get();
    bool bWasActive = false;   // 살아 있던 러너 / 술래였는지(유령이었으면 조용히 정리만)
    if (AMyCharacter* MC = Cast<AMyCharacter>(Pawn))
    {
        bWasActive = !MC->IsEliminated();
        if (MC->IsPossessing())
        {
            MC->ForceReleasePossessedProp_Server();
        }
    }
    if (IsValid(Pawn) && !Pawn->GetController())
    {
        Pawn->Destroy();
    }

    if (!bWasActive)
    {
        return;
    }

    // 안내(이름은 기록에 남은 닉네임으로)
    const bool bRunner = Record.Data.FinalRole == EFinalRole::Runner;
    const FString SubText = bRunner
        ? (CountActiveRunners() > 0 ? FString::Printf(TEXT("남은 러너 %d명"), CountActiveRunners()) : FString(TEXT("러너 전멸!")))
        : FString();
    if (GetWorld())
    {
        const FString Name = Record.Data.Nickname.IsEmpty() ? TEXT("플레이어") : Record.Data.Nickname;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (AMyPlayerController* MPC = Cast<AMyPlayerController>(It->Get()))
            {
                MPC->ClientShowConnectionNotice(Name, EConnectionNoticeKind::DroppedOut, SubText);
            }
        }
    }

    // 마지막 러너였으면 술래 승리
    if (bRunner && !HasAnyActiveRunner())
    {
        EndGameWithWinnerDelayed(EFinalRole::Seeker, EliminationResultDelaySeconds);
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

            // 이 사람만의 재접속 시간. 지나도 안 돌아오면 캐릭터를 정리한다(살아 있는 러너로 계속 세어지지 않게).
            if (ReconnectGraceSeconds > 0.f)
            {
                FTimerHandle& Handle = PlayerReconnectTimers.FindOrAdd(Key);
                GetWorldTimerManager().SetTimer(Handle,
                    FTimerDelegate::CreateUObject(this, &AMyGameMode::OnPlayerReconnectExpired, Key),
                    ReconnectGraceSeconds, false);
            }

            BroadcastConnectionNotice(ExitingPlayerState, EConnectionNoticeKind::Disconnected,
                ReconnectGraceSeconds > 0.f
                    ? FString::Printf(TEXT("%d초 안에 돌아오면 이어서 해요"), FMath::RoundToInt(ReconnectGraceSeconds))
                    : FString());
        }
    }
    else if (!bGameEnded && ExitingPlayerState && ExitingPlayerState->GetFinalRole() != EFinalRole::None
        && ExitingPC && !ExitingPC->IsLocalController())
    {
        BroadcastConnectionNotice(ExitingPlayerState, EConnectionNoticeKind::Left, FString());
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

            // 남은 사람들 화면에 "술래가 모두 나갔어요 · 0:45" 카운트다운을 띄운다
            if (AMyGameState* GS = GetGameState<AMyGameState>())
            {
                GS->SetReconnectWait_Server(RemainingSeekers == 0 ? EFinalRole::Seeker : EFinalRole::Runner, ReconnectGraceSeconds);
            }
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

    // 여기서는 Pawn을 만들거나 역할을 정하지 않는다. 플레이어별 처리는 HandleSeamlessTravelPlayer에서 한다.
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

    // 플레이 종료로 모두 로그아웃되는 중(월드 정리 중)에는 승패 처리를 하지 않는다
    if (!GetWorld() || GetWorld()->bIsTearingDown)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("EndGameWithWinner: WinningRole=%d"), static_cast<int32>(WinningRole));

    bGameEnded = true;

    GetWorldTimerManager().ClearTimer(TH_GamePhase);
    GetWorldTimerManager().ClearTimer(TH_CloseHideScore);
    GetWorldTimerManager().ClearTimer(TH_StallTaunt);
    GetWorldTimerManager().ClearTimer(TH_ReconnectGrace);
    for (TPair<FString, FTimerHandle>& Pair : PlayerReconnectTimers)
    {
        GetWorldTimerManager().ClearTimer(Pair.Value);
    }
    PlayerReconnectTimers.Empty();
    if (AMyGameState* WaitGS = GetGameState<AMyGameState>())
    {
        WaitGS->SetReconnectWait_Server(EFinalRole::None, 0.0);
    }

    AwardWinBonus(WinningRole);

    // 게임이 끝났으니 모두의 점수를 공개한다(이후로는 점수가 더 오르지 않는다)
    GetWorldTimerManager().ClearTimer(TH_TeamScores);
    for (APlayerState* PS : GameState ? GameState->PlayerArray : TArray<TObjectPtr<APlayerState>>())
    {
        if (AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS))
        {
            MyPS->RevealScores_Server();
        }
    }

    if (AMyGameState* GS = GetGameState<AMyGameState>())
    {
        // 종료 페이즈 남은 시간 = 대기실 복귀까지 남은 시간(결과 화면 카운트다운이 이 값을 읽는다)
        const double ReturnDelay = (bAutoReturnToWaitRoom && ReturnToWaitRoomDelaySeconds > 0.f)
            ? static_cast<double>(ReturnToWaitRoomDelaySeconds)
            : 0.0;
        GS->SetGamePhase_Server(EGamePhase::Ended, ReturnDelay);
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

void AMyGameMode::RequestReturnToWaitRoom(APlayerController* Requester)
{
    if (!HasAuthority() || !bGameEnded || !Requester)
    {
        return;
    }

    const AMyPlayerState* PS = Requester->GetPlayerState<AMyPlayerState>();
    if (!PS || !PS->IsHost())
    {
        UE_LOG(LogTemp, Log, TEXT("Ignored return-to-wait-room request from a non-host player."));
        return;
    }

    ReturnToWaitRoom();
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

    // 반드시 상대 경로(bAbsolute=false)로 이동한다.
    // 절대 경로 이동은 이전 주소의 포트와 옵션(listen 등)을 버리고 기본 포트(7777)로 다시 열린다.
    // 클라이언트는 원래 접속했던 주소/포트(PIE는 17777 등)로 재접속하므로 포트가 달라지면 대기실로 못 들어온다.
    // 상대 경로는 포트와 listen을 그대로 유지하고, ?game= 옵션은 새 값으로 덮어쓴다.
    FString TravelURL = WaitRoomTravelURL;
    if (!TravelURL.Contains(TEXT("listen"), ESearchCase::IgnoreCase))
    {
        TravelURL += TEXT("?listen");
    }

    GetWorld()->ServerTravel(TravelURL, false);
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

    // 전체 타임라인(단계 순서와 시간)을 클라이언트에 알려 시간 바가 이 값으로 그려지게 한다
    if (AMyGameState* GS = GetGameState<AMyGameState>())
    {
        const TPair<EGamePhase, double> Phases[] = {
            { EGamePhase::HideTime, HideTimeSeconds },
            { EGamePhase::Phase1, Phase1Seconds },
            { EGamePhase::ForcedSwap, ForcedSwapSeconds },
            { EGamePhase::Phase2, Phase2Seconds },
            { EGamePhase::FeverTime, FeverTimeSeconds },
        };

        TArray<FPhaseTimelineEntry> Timeline;
        for (const TPair<EGamePhase, double>& P : Phases)
        {
            FPhaseTimelineEntry& Entry = Timeline.AddDefaulted_GetRef();
            Entry.Phase = P.Key;
            Entry.DurationSeconds = static_cast<float>(FMath::Max(P.Value, 0.0));
        }
        GS->SetPhaseTimeline_Server(Timeline);
    }

    EnterGamePhase(EGamePhase::HideTime, HideTimeSeconds);
    GetWorldTimerManager().SetTimer(TH_GamePhase, this, &AMyGameMode::EnterPhase1, HideTimeSeconds, false);

    // 팀원 점수는 같은 팀에게만 주기적으로 보낸다
    GetWorldTimerManager().SetTimer(TH_TeamScores, this, &AMyGameMode::PushTeamScores, 0.5f, true, 0.5f);
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

    // 술래가 움직이기 시작한 뒤부터 1초마다 "코앞에서 숨기" 점수를 계산한다
    GetWorldTimerManager().SetTimer(TH_CloseHideScore, this, &AMyGameMode::TickCloseHideScore, 1.f, true);
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

    // 시간 끌기 방지: 피버 타임 동안 숨는 사람 위치에서 주기적으로 소리
    if (StallTauntSound)
    {
        GetWorldTimerManager().SetTimer(TH_StallTaunt, this, &AMyGameMode::PlayStallTaunts,
            StallTauntIntervalSeconds, true, StallTauntFirstDelaySeconds);
    }
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

// ===== 점수 =====

void AMyGameMode::TickCloseHideScore()
{
    if (!HasAuthority() || bGameEnded || !GetWorld())
    {
        return;
    }

    TArray<AMyCharacter*> Seekers;
    for (TActorIterator<AMyCharacter> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It) && It->IsSeeker() && !It->IsEliminated())
        {
            Seekers.Add(*It);
        }
    }

    if (Seekers.Num() == 0)
    {
        return;
    }

    for (TActorIterator<AMyCharacter> It(GetWorld()); It; ++It)
    {
        AMyCharacter* Runner = *It;
        if (!IsValid(Runner) || Runner->IsSeeker() || Runner->IsEliminated())
        {
            continue;
        }

        // 물체로 숨어 있을 때만(그냥 서 있는 건 숨은 게 아니다)
        APropBase* Hidden = Runner->GetPossessedProp();
        if (!IsValid(Hidden))
        {
            continue;
        }

        FVector PropCenter;
        FVector PropExtent;
        Hidden->GetActorBounds(true, PropCenter, PropExtent);

        for (AMyCharacter* Seeker : Seekers)
        {
            if (FVector::DistSquared(Seeker->GetActorLocation(), PropCenter) > FMath::Square(CloseHideRadius))
            {
                continue;
            }

            // 술래 눈에서 물체까지 가리는 게 없어야 한다(벽 너머는 "코앞"이 아니다)
            FCollisionQueryParams Params(SCENE_QUERY_STAT(CloseHideTrace), true);
            Params.AddIgnoredActor(Seeker);
            Params.AddIgnoredActor(Runner);

            FHitResult Hit;
            const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
                Hit, Seeker->GetPawnViewLocation(), PropCenter, ECC_Visibility, Params);
            if (bBlocked && Hit.GetActor() != Hidden)
            {
                continue;
            }

            if (AMyPlayerState* PS = Runner->GetPlayerState<AMyPlayerState>())
            {
                PS->AddScore_Server(EScoreReason::CloseHide);
            }
            break;
        }
    }
}

void AMyGameMode::AwardWinBonus(EFinalRole WinningRole)
{
    if (!HasAuthority() || WinningRole == EFinalRole::None)
    {
        return;
    }

    if (AGameStateBase* GS = GameState)
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
            AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
            if (MyPS && MyPS->GetFinalRole() == WinningRole)
            {
                MyPS->AddScore_Server(EScoreReason::Win);
            }
        }
    }
}

// ===== 시간 끌기 방지 =====

void AMyGameMode::PlayStallTaunts()
{
    if (!HasAuthority() || bGameEnded || !GetWorld() || !StallTauntSound)
    {
        GetWorldTimerManager().ClearTimer(TH_StallTaunt);
        return;
    }

    for (TActorIterator<AMyCharacter> It(GetWorld()); It; ++It)
    {
        AMyCharacter* Runner = *It;
        if (IsValid(Runner) && !Runner->IsSeeker() && !Runner->IsEliminated())
        {
            Runner->MulticastPlayStallTaunt(StallTauntSound);
        }
    }
}

void AMyGameMode::PushTeamScores()
{
    if (!HasAuthority() || bGameEnded || !GameState)
    {
        return;
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AMyPlayerController* MPC = Cast<AMyPlayerController>(It->Get());
        const AMyPlayerState* ViewerPS = MPC ? MPC->GetPlayerState<AMyPlayerState>() : nullptr;
        if (!ViewerPS || ViewerPS->GetFinalRole() == EFinalRole::None)
        {
            continue;
        }

        TArray<FTeamScoreEntry> Entries;
        for (APlayerState* PS : GameState->PlayerArray)
        {
            AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
            if (MyPS && MyPS->GetFinalRole() == ViewerPS->GetFinalRole())
            {
                FTeamScoreEntry& Entry = Entries.AddDefaulted_GetRef();
                Entry.Player = MyPS;
                Entry.Stats = MyPS->GetScoreStats();
            }
        }

        MPC->ClientReceiveTeamScores(Entries);
    }
}
