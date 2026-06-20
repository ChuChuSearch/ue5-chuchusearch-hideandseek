#include "WaitRoomGameMode.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "MyPlayerController.h"

AWaitRoomGameMode::AWaitRoomGameMode()
{
    PlayerStateClass = AMyPlayerState::StaticClass();
    DefaultPawnClass = nullptr;
    bUseSeamlessTravel = true;
}

void AWaitRoomGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    EnsureHost();
}

void AWaitRoomGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);

    EnsureHost();
}

void AWaitRoomGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
    Super::HandleSeamlessTravelPlayer(C);

    if (AMyPlayerController* PC = Cast<AMyPlayerController>(C))
    {
        PC->ClientEnterWaitRoomUI();
    }

    EnsureHost();
}

void AWaitRoomGameMode::EnsureHost()
{
    if (!HasAuthority() || !GameState) return;

    AMyPlayerState* CurrentHost = nullptr;
    TArray<AMyPlayerState*> AllPS;
    AllPS.Reserve(GameState->PlayerArray.Num());

    for (APlayerState* PS : GameState->PlayerArray)
    {
        AMyPlayerState* MPS = Cast<AMyPlayerState>(PS);
        if (!MPS) continue;

        AllPS.Add(MPS);
        if (MPS->IsHost())
        {
            CurrentHost = MPS;
        }
    }

    if (AllPS.Num() == 0) return;

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
    if (!HasAuthority() || !IsValid(RequestPC) || !GameState) return;

    AMyPlayerState* RequestPS = RequestPC->GetPlayerState<AMyPlayerState>();
    if (!RequestPS || !RequestPS->IsHost()) return;

    const int32 PlayerCount = GameState->PlayerArray.Num();
    if (PlayerCount < 2 || PlayerCount > 4) return;

    AMyPlayerState* SeekerPS = nullptr;
    if (!PickSeeker(SeekerPS) || !SeekerPS) return;

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
    GetWorld()->ServerTravel(TravelURL, false);
}

