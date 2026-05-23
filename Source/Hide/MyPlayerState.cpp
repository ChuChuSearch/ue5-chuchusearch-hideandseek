#include "MyPlayerState.h"
#include "MyPlayerController.h"
#include "Net/UnrealNetwork.h"

AMyPlayerState::AMyPlayerState()
{
    bReplicates = true;
}

void AMyPlayerState::ServerSetNickname_Implementation(const FString& InNickname)
{
    if (!HasAuthority()) return;

    FString Trimmed = InNickname;
    Trimmed.TrimStartAndEndInline();

    Nickname = Trimmed.Left(16);
}

void AMyPlayerState::ServerSetPreference_Implementation(ERolePreference InPreference)
{
    if (!HasAuthority()) return;

    Preference = InPreference;
}

void AMyPlayerState::SetHost_Server(bool bNewHost)
{
    if (!HasAuthority()) return;

    bIsHost = bNewHost;
}

void AMyPlayerState::SetFinalRole_Server(EFinalRole InRole)
{
    if (!HasAuthority()) return;

    FinalRole = InRole;
}

void AMyPlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);

    AMyPlayerState* MyPS = Cast<AMyPlayerState>(PlayerState);
    if (!MyPS) return;

    MyPS->Nickname = Nickname;
    MyPS->Preference = Preference;
    MyPS->FinalRole = FinalRole;
    MyPS->bIsHost = bIsHost;
}

void AMyPlayerState::OverrideWith(APlayerState* PlayerState)
{
    Super::OverrideWith(PlayerState);

    const AMyPlayerState* MyPS = Cast<AMyPlayerState>(PlayerState);
    if (!MyPS) return;

    Nickname = MyPS->Nickname;
    Preference = MyPS->Preference;
    FinalRole = MyPS->FinalRole;
    bIsHost = MyPS->bIsHost;
}

void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMyPlayerState, Nickname);
    DOREPLIFETIME(AMyPlayerState, Preference);
    DOREPLIFETIME(AMyPlayerState, FinalRole);
    DOREPLIFETIME(AMyPlayerState, bIsHost);
    DOREPLIFETIME(AMyPlayerState, CollectedClueNumbers);
}

bool AMyPlayerState::HasCollectedClue(int32 InClueNumber) const
{
    return CollectedClueNumbers.Contains(InClueNumber);
}

void AMyPlayerState::ServerCollectClue_Implementation(int32 InClueNumber)
{
    if (!HasAuthority()) return;
    if (InClueNumber < 0 || InClueNumber > 9) return;

    if (CollectedClueNumbers.Contains(InClueNumber))
    {
        return;
    }

    CollectedClueNumbers.Add(InClueNumber);
    NotifyClueCollectionChanged();

}

void AMyPlayerState::OnRep_CollectedClueNumbers()
{
    NotifyClueCollectionChanged();
}

void AMyPlayerState::NotifyClueCollectionChanged()
{
    AMyPlayerController* MyPC = Cast<AMyPlayerController>(GetOwner());
    if (!MyPC)
    {
        return;
    }

    MyPC->UpdateGameRoomClues(CollectedClueNumbers);
}

