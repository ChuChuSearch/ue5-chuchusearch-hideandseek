#include "WaitRoomGameMode.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "MyPlayerController.h"
#include "TimerManager.h"
#include "MyGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"

AWaitRoomGameMode::AWaitRoomGameMode()
{
    PlayerStateClass = AMyPlayerState::StaticClass();
    DefaultPawnClass = nullptr;
    bUseSeamlessTravel = true;
}

void AWaitRoomGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    InitializeLobbySettingsDefaults();
}

void AWaitRoomGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (!ErrorMessage.IsEmpty())
    {
        return;
    }

    const int32 MaxPlayers = GetMaxLobbyPlayers();
    if (MaxPlayers > 0 && GetNumPlayers() >= MaxPlayers)
    {
        ErrorMessage = TEXT("Server full");
        UE_LOG(LogTemp, Log, TEXT("Rejected lobby login from %s: lobby full (%d/%d)."), *Address, GetNumPlayers(), MaxPlayers);
    }
}

void AWaitRoomGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    RegisterLobbyJoinOrder(NewPlayer ? NewPlayer->GetPlayerState<AMyPlayerState>() : nullptr);
    EnsureHost();

    // 참가자 목록("N/최대")과 게임 설정 창에 보여 줄 지금 설정을 알려 준다
    SendLobbyGameSettings(NewPlayer);
}

// ===== 방장 게임 설정 =====

ULobbySettingsSubsystem* AWaitRoomGameMode::GetSettingsSubsystem() const
{
    const UGameInstance* GI = GetGameInstance();
    return GI ? GI->GetSubsystem<ULobbySettingsSubsystem>() : nullptr;
}

void AWaitRoomGameMode::InitializeLobbySettingsDefaults()
{
    ULobbySettingsSubsystem* Sub = GetSettingsSubsystem();
    if (!Sub || Sub->IsInitialized())
    {
        return;
    }

    FLobbyGameSettings Defaults;
    Defaults.MaxPlayers = MaxLobbyPlayers;

    // 게임 맵에서 쓸 게임 모드(…?game=…_C)의 블루프린트 기본값에서 단계 시간을 읽는다
    if (UClass* GameClass = LoadGameModeClassFromTravelURL())
    {
        GameClass->GetDefaultObject<AMyGameMode>()->FillPhaseSettings(Defaults);
    }

    Sub->InitializeDefaults(Defaults);
}

UClass* AWaitRoomGameMode::LoadGameModeClassFromTravelURL() const
{
    const FString URL = BuildGameTravelURL();
    int32 OptionStart = INDEX_NONE;
    if (!URL.FindChar(TEXT('?'), OptionStart))
    {
        return nullptr;
    }

    const FString GameClassPath = UGameplayStatics::ParseOption(URL.Mid(OptionStart), TEXT("game"));
    return GameClassPath.IsEmpty() ? nullptr : LoadClass<AMyGameMode>(nullptr, *GameClassPath);
}

UClass* AWaitRoomGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    if (LobbyPawnClass)
    {
        return LobbyPawnClass;
    }

    // 게임 맵과 같은 캐릭터로 로비 맵을 돌아다닌다(처음 한 번 찾아서 기억)
    if (!DefaultPawnClass)
    {
        if (UClass* GameClass = LoadGameModeClassFromTravelURL())
        {
            DefaultPawnClass = GameClass->GetDefaultObject<AMyGameMode>()->GetPlayerCharacterClass();
        }
    }

    return DefaultPawnClass;
}

void AWaitRoomGameMode::DestroyLobbyPawns()
{
    if (!GetWorld())
    {
        return;
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        APawn* Pawn = PC ? PC->GetPawn() : nullptr;
        if (Pawn)
        {
            PC->UnPossess();
            Pawn->Destroy();
        }
    }
}

FLobbyGameSettings AWaitRoomGameMode::GetLobbyGameSettings() const
{
    if (const ULobbySettingsSubsystem* Sub = GetSettingsSubsystem())
    {
        return Sub->GetSettings();
    }

    FLobbyGameSettings Fallback;
    Fallback.MaxPlayers = MaxLobbyPlayers;
    return Fallback;
}

int32 AWaitRoomGameMode::GetMaxLobbyPlayers() const
{
    return GetLobbyGameSettings().MaxPlayers;
}

void AWaitRoomGameMode::SendLobbyGameSettings(APlayerController* PC) const
{
    if (AMyPlayerController* MPC = Cast<AMyPlayerController>(PC))
    {
        MPC->ClientSetLobbyGameSettings(GetLobbyGameSettings());
    }
}

void AWaitRoomGameMode::BroadcastLobbyGameSettings()
{
    if (!GetWorld())
    {
        return;
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        SendLobbyGameSettings(It->Get());
    }
}

void AWaitRoomGameMode::HandleSetLobbyGameSettings(APlayerController* RequestPC, const FLobbyGameSettings& NewSettings, bool bResetToDefaults)
{
    ULobbySettingsSubsystem* Sub = GetSettingsSubsystem();
    const AMyPlayerState* RequestPS = RequestPC ? RequestPC->GetPlayerState<AMyPlayerState>() : nullptr;
    if (!HasAuthority() || !Sub || !RequestPS || !RequestPS->IsHost())
    {
        return;
    }

    FLobbyGameSettings Wanted = FLobbySettingRules::Sanitize(bResetToDefaults ? Sub->GetDefaults() : NewSettings);

    // 지금 들어와 있는 사람보다 최대 인원을 줄일 수는 없다
    const int32 CurrentPlayers = GameState ? GameState->PlayerArray.Num() : 0;
    if (Wanted.MaxPlayers < CurrentPlayers)
    {
        Wanted.MaxPlayers = FLobbySettingRules::SnapToOption(ELobbySettingField::MaxPlayers, CurrentPlayers);
        if (Wanted.MaxPlayers < CurrentPlayers)
        {
            Wanted.MaxPlayers = Sub->GetSettings().MaxPlayers;
        }
    }

    if (Wanted == Sub->GetSettings())
    {
        // 바뀐 게 없어도(예: 인원 때문에 막힘) 화면을 맞추도록 다시 보낸다
        SendLobbyGameSettings(RequestPC);
        return;
    }

    Sub->SetSettings(Wanted);
    BroadcastLobbyGameSettings();

    // 조건이 바뀌었으니 준비를 모두 푼다
    if (GameState)
    {
        for (APlayerState* PS : GameState->PlayerArray)
        {
            if (AMyPlayerState* MPS = Cast<AMyPlayerState>(PS))
            {
                MPS->ResetReady_Server();
            }
        }
    }

    // 연달아 바꾸는 동안에는 미루다가 마지막에 한 번만 알린다
    GetWorldTimerManager().SetTimer(TH_SettingsChangedNotice, this, &AWaitRoomGameMode::AnnounceSettingsChanged, 1.5f, false);
}

void AWaitRoomGameMode::AnnounceSettingsChanged()
{
    const FLobbyGameSettings S = GetLobbyGameSettings();
    BroadcastLobbyChat(FString(), FString::Printf(TEXT("방장이 게임 설정을 바꿨어요 (최대 %d명 · 전체 %s) 준비를 다시 해 주세요"),
        S.MaxPlayers, *FLobbySettingRules::FormatTime(S.GetTotalSeconds())), true);
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
        SendLobbyGameSettings(PC);
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

bool AWaitRoomGameMode::CheckStartConditions(APlayerController* RequestPC, FString* OutFailReason) const
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

    const int32 MaxPlayers = GetMaxLobbyPlayers();
    if (MaxPlayers > 0 && PlayerCount > MaxPlayers) return false;

    // 방장을 뺀 모두가 준비해야 시작할 수 있다
    for (APlayerState* PS : GameState->PlayerArray)
    {
        const AMyPlayerState* MPS = Cast<AMyPlayerState>(PS);
        if (MPS && !MPS->IsHost() && !MPS->IsReady())
        {
            return Fail(TEXT("아직 출발 발판에 안 올라온 사람이 있어요"));
        }
    }

    return true;
}

bool AWaitRoomGameMode::TryStartGame(APlayerController* RequestPC, FString* OutFailReason)
{
    if (!CheckStartConditions(RequestPC, OutFailReason))
    {
        return false;
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
    if (!GetWorld()->ServerTravel(TravelURL, false))
    {
        return false;
    }

    // 이동은 다음 틱부터 시작된다. 로비 캐릭터가 게임 맵으로 따라가지 않도록 지금 치운다
    // (게임 맵에서는 역할에 맞춰 새로 만든다).
    DestroyLobbyPawns();
    return true;
}

