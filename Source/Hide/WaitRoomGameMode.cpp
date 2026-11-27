#include "WaitRoomGameMode.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "MyPlayerController.h"
#include "TimerManager.h"

AWaitRoomGameMode::AWaitRoomGameMode()
{
    PlayerStateClass = AMyPlayerState::StaticClass();
    DefaultPawnClass = nullptr;
    bUseSeamlessTravel = true;
}

void AWaitRoomGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (!ErrorMessage.IsEmpty())
    {
        return;
    }

    if (MaxLobbyPlayers > 0 && GetNumPlayers() >= MaxLobbyPlayers)
    {
        ErrorMessage = TEXT("Server full");
        UE_LOG(LogTemp, Log, TEXT("Rejected lobby login from %s: lobby full (%d/%d)."), *Address, GetNumPlayers(), MaxLobbyPlayers);
    }
}

void AWaitRoomGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    RegisterLobbyJoinOrder(NewPlayer ? NewPlayer->GetPlayerState<AMyPlayerState>() : nullptr);
    EnsureHost();

    // 참가자 목록에 "N/최대" 로 보여 주도록 최대 인원을 알려 준다
    if (AMyPlayerController* MPC = Cast<AMyPlayerController>(NewPlayer))
    {
        MPC->ClientSetLobbyMaxPlayers(MaxLobbyPlayers);
    }
}

void AWaitRoomGameMode::Logout(AController* Exiting)
{
    const AMyPlayerState* ExitingPlayerState = Exiting
        ? Exiting->GetPlayerState<AMyPlayerState>()
        : nullptr;

    const bool bWasHost = ExitingPlayerState && ExitingPlayerState->IsHost();
    const FString ExitingName = ExitingPlayerState ? ExitingPlayerState->GetNickname() : FString();

    LastChatTime.Remove(Cast<APlayerController>(Exiting));

    Super::Logout(Exiting);

    EnsureHost(ExitingPlayerState);

    if (!ExitingName.IsEmpty())
    {
        BroadcastLobbyChat(FString(), FString::Printf(TEXT("%s님이 나갔어요"), *ExitingName), true);
    }

    // 방장이 나가서 다른 사람이 방장이 됐으면 알려 준다
    if (bWasHost && GameState)
    {
        for (APlayerState* PS : GameState->PlayerArray)
        {
            const AMyPlayerState* MPS = Cast<AMyPlayerState>(PS);
            if (MPS && MPS != ExitingPlayerState && MPS->IsHost() && !MPS->GetNickname().IsEmpty())
            {
                BroadcastLobbyChat(FString(), FString::Printf(TEXT("%s님이 새 방장이 됐어요"), *MPS->GetNickname()), true);
                break;
            }
        }
    }
}

void AWaitRoomGameMode::NotifyNicknameSet(AMyPlayerState* PS)
{
    if (!HasAuthority() || !PS || AnnouncedPlayers.Contains(PS))
    {
        return;
    }

    FTimerHandle& Handle = JoinAnnounceTimers.FindOrAdd(PS);
    FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &AWaitRoomGameMode::AnnounceJoin, TWeakObjectPtr<AMyPlayerState>(PS));
    GetWorldTimerManager().SetTimer(Handle, Delegate, 1.5f, false);
}

void AWaitRoomGameMode::AnnounceJoin(TWeakObjectPtr<AMyPlayerState> PS)
{
    JoinAnnounceTimers.Remove(PS);
    if (!PS.IsValid() || AnnouncedPlayers.Contains(PS) || PS->GetNickname().IsEmpty())
    {
        return;
    }

    AnnouncedPlayers.Add(PS);
    BroadcastLobbyChat(FString(), FString::Printf(TEXT("%s님이 들어왔어요"), *PS->GetNickname()), true);
}

void AWaitRoomGameMode::HandleLobbyChat(APlayerController* SenderPC, const FString& RawMessage)
{
    if (!HasAuthority() || !SenderPC)
    {
        return;
    }

    // 도배 방지
    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (const double* Last = LastChatTime.Find(SenderPC))
    {
        if (Now - *Last < ChatMinIntervalSeconds)
        {
            return;
        }
    }

    // 제어 문자(개행 등) 제거, 앞뒤 공백 정리, 길이 제한
    FString Message;
    Message.Reserve(RawMessage.Len());
    for (const TCHAR Ch : RawMessage)
    {
        if (!FChar::IsControl(Ch))
        {
            Message.AppendChar(Ch);
        }
    }
    Message.TrimStartAndEndInline();
    Message = Message.Left(MaxChatLength);
    if (Message.IsEmpty())
    {
        return;
    }

    LastChatTime.Add(SenderPC, Now);

    const AMyPlayerState* PS = SenderPC->GetPlayerState<AMyPlayerState>();
    FString SenderName = PS ? PS->GetNickname() : FString();
    if (SenderName.IsEmpty())
    {
        SenderName = TEXT("이름 없음");
    }

    BroadcastLobbyChat(SenderName, Message, false);
}

void AWaitRoomGameMode::BroadcastLobbyChat(const FString& SenderName, const FString& Message, bool bSystem)
{
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AMyPlayerController* PC = Cast<AMyPlayerController>(It->Get()))
        {
            PC->ClientReceiveLobbyChat(SenderName, Message, bSystem);
        }
    }
}

void AWaitRoomGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
    Super::HandleSeamlessTravelPlayer(C);

    if (AMyPlayerController* PC = Cast<AMyPlayerController>(C))
    {
        RegisterLobbyJoinOrder(PC->GetPlayerState<AMyPlayerState>());
        PC->ClientEnterWaitRoomUI();
        PC->ClientSetLobbyMaxPlayers(MaxLobbyPlayers);
    }

    EnsureHost();
}

void AWaitRoomGameMode::RegisterLobbyJoinOrder(AMyPlayerState* PlayerState)
{
    if (!HasAuthority() || !PlayerState)
    {
        return;
    }

    if (PlayerState->GetLobbyJoinOrder() <= 0)
    {
        PlayerState->SetLobbyJoinOrder_Server(NextLobbyJoinOrder++);
        return;
    }

    NextLobbyJoinOrder = FMath::Max(NextLobbyJoinOrder, PlayerState->GetLobbyJoinOrder() + 1);
}

void AWaitRoomGameMode::EnsureHost(const AMyPlayerState* ExcludedPlayerState)
{
    if (!HasAuthority() || !GameState) return;

    AMyPlayerState* CurrentHost = nullptr;
    TArray<AMyPlayerState*> AllPS;
    AllPS.Reserve(GameState->PlayerArray.Num());

    for (APlayerState* PS : GameState->PlayerArray)
    {
        AMyPlayerState* MPS = Cast<AMyPlayerState>(PS);
        if (!MPS || MPS == ExcludedPlayerState) continue;

        AllPS.Add(MPS);
        if (MPS->IsHost())
        {
            CurrentHost = MPS;
        }
    }

    if (AllPS.Num() == 0) return;

    AllPS.Sort([](const AMyPlayerState& A, const AMyPlayerState& B)
    {
        const int32 AOrder = A.GetLobbyJoinOrder() > 0 ? A.GetLobbyJoinOrder() : MAX_int32;
        const int32 BOrder = B.GetLobbyJoinOrder() > 0 ? B.GetLobbyJoinOrder() : MAX_int32;
        return AOrder < BOrder;
    });

    if (!CurrentHost)
    {
        AllPS[0]->SetHost_Server(true);
        for (int32 i = 1; i < AllPS.Num(); ++i)
        {
            AllPS[i]->SetHost_Server(false);
        }
        return;
    }

    for (AMyPlayerState* MPS : AllPS)
    {
        MPS->SetHost_Server(MPS == CurrentHost);
    }
}

bool AWaitRoomGameMode::PickSeeker(AMyPlayerState*& OutSeekerPS) const
{
    OutSeekerPS = nullptr;
    if (!GameState) return false;

    TArray<AMyPlayerState*> SeekerWish;
    TArray<AMyPlayerState*> Any;
    TArray<AMyPlayerState*> Everyone;

    for (APlayerState* PS : GameState->PlayerArray)
    {
        AMyPlayerState* MPS = Cast<AMyPlayerState>(PS);
        if (!MPS) continue;

        Everyone.Add(MPS);

        switch (MPS->GetPreference())
        {
        case ERolePreference::SeekerWish: SeekerWish.Add(MPS); break;
        case ERolePreference::Any:        Any.Add(MPS);        break;
        case ERolePreference::RunnerWish:                         break;
        default:                                                break;
        }
    }

    auto PickRandom = [](const TArray<AMyPlayerState*>& Arr) -> AMyPlayerState*
        {
            if (Arr.Num() <= 0) return nullptr;
            const int32 Idx = FMath::RandRange(0, Arr.Num() - 1);
            return Arr.IsValidIndex(Idx) ? Arr[Idx] : nullptr;
        };

    if (AMyPlayerState* Pick = PickRandom(SeekerWish)) { OutSeekerPS = Pick; return true; }
    if (AMyPlayerState* Pick = PickRandom(Any)) { OutSeekerPS = Pick; return true; }
    if (AMyPlayerState* Pick = PickRandom(Everyone)) { OutSeekerPS = Pick; return true; }

    return false;
}

FString AWaitRoomGameMode::BuildGameTravelURL() const
{
    if (!GameMapTravelURL.IsEmpty())
    {
        return GameMapTravelURL;
    }

    if (!GameMapAsset.IsNull())
    {
        // "/Game/Game/Levels/GameMap"
        return GameMapAsset.ToSoftObjectPath().GetLongPackageName();
    }

    return TEXT("/Game/Game/Levels/GameMap");
}

void AWaitRoomGameMode::StartGame(APlayerController* RequestPC)
{
    TryStartGame(RequestPC);
}

bool AWaitRoomGameMode::TryStartGame(APlayerController* RequestPC, FString* OutFailReason)
{
    auto Fail = [OutFailReason](const TCHAR* Reason)
    {
        if (OutFailReason)
        {
            *OutFailReason = Reason;
        }
        return false;
    };

    if (!HasAuthority() || !IsValid(RequestPC) || !GameState) return false;

    AMyPlayerState* RequestPS = RequestPC->GetPlayerState<AMyPlayerState>();
    if (!RequestPS || !RequestPS->IsHost()) return Fail(TEXT("게임은 방장만 시작할 수 있어요"));

    const int32 PlayerCount = GameState->PlayerArray.Num();
    if (PlayerCount < 2)
    {
        return Fail(TEXT("2명 이상 모여야 시작할 수 있어요"));
    }

    if (MaxLobbyPlayers > 0 && PlayerCount > MaxLobbyPlayers) return false;

    // 방장을 뺀 모두가 준비해야 시작할 수 있다
    for (APlayerState* PS : GameState->PlayerArray)
    {
        const AMyPlayerState* MPS = Cast<AMyPlayerState>(PS);
        if (MPS && !MPS->IsHost() && !MPS->IsReady())
        {
            return Fail(TEXT("아직 준비하지 않은 사람이 있어요"));
        }
    }

    AMyPlayerState* SeekerPS = nullptr;
    if (!PickSeeker(SeekerPS) || !SeekerPS) return false;

    for (APlayerState* PS : GameState->PlayerArray)
    {
        AMyPlayerState* MPS = Cast<AMyPlayerState>(PS);
        if (!MPS) continue;

        const EFinalRole NewFinalRole = (MPS == SeekerPS)
            ? EFinalRole::Seeker
            : EFinalRole::Runner;

        MPS->SetFinalRole_Server(NewFinalRole);
    }

    const FString TravelURL = BuildGameTravelURL();
    return GetWorld()->ServerTravel(TravelURL, false);
}

