#include "MyPlayerState.h"
#include "MyPlayerController.h"
#include "MyGameState.h"
#include "WaitRoomGameMode.h"
#include "Net/UnrealNetwork.h"

AMyPlayerState::AMyPlayerState()
{
    bReplicates = true;
}

bool AMyPlayerState::IsInWaitRoom_Server() const
{
    const UWorld* World = GetWorld();
    return World && World->GetAuthGameMode<AWaitRoomGameMode>() != nullptr;
}

void AMyPlayerState::ServerSetNickname_Implementation(const FString& InNickname)
{
    if (!HasAuthority()) return;

    // 게임 도중 닉네임 변경(다른 사람 사칭 등) 방지: 대기실에서만 허용
    if (!IsInWaitRoom_Server()) return;

    // 제어 문자(개행, 탭, 널 등) 제거 후 공백 정리
    FString Sanitized;
    Sanitized.Reserve(InNickname.Len());
    for (const TCHAR Ch : InNickname)
    {
        if (!FChar::IsControl(Ch))
        {
            Sanitized.AppendChar(Ch);
        }
    }

    Sanitized.TrimStartAndEndInline();
    Sanitized = Sanitized.Left(16);

    if (Sanitized.IsEmpty())
    {
        return;
    }

    Nickname = Sanitized;
    ForceNetUpdate();
}

void AMyPlayerState::ServerSetPreference_Implementation(ERolePreference InPreference)
{
    if (!HasAuthority()) return;
    if (!IsInWaitRoom_Server()) return;

    switch (InPreference)
    {
    case ERolePreference::SeekerWish:
    case ERolePreference::RunnerWish:
    case ERolePreference::Any:
        Preference = InPreference;
        ForceNetUpdate();
        break;
    default:
        // 정의되지 않은 enum 값은 무시
        break;
    }
}

void AMyPlayerState::ExportReconnectData(FReconnectPlayerData& OutData) const
{
    OutData.Nickname = Nickname;
    OutData.Preference = Preference;
    OutData.FinalRole = FinalRole;
    OutData.bIsHost = bIsHost;
    OutData.LobbyJoinOrder = LobbyJoinOrder;
    OutData.CollectedClueNumbers = CollectedClueNumbers;
    OutData.PositionedClueNumbers = PositionedClueNumbers;
}

void AMyPlayerState::ImportReconnectData_Server(const FReconnectPlayerData& InData)
{
    if (!HasAuthority()) return;

    Nickname = InData.Nickname;
    Preference = InData.Preference;
    FinalRole = InData.FinalRole;
    bIsHost = InData.bIsHost;
    LobbyJoinOrder = InData.LobbyJoinOrder;
    CollectedClueNumbers = InData.CollectedClueNumbers;
    PositionedClueNumbers = InData.PositionedClueNumbers;

    ForceNetUpdate();
    NotifyClueCollectionChanged();
}

void AMyPlayerState::SetHost_Server(bool bNewHost)
{
    if (!HasAuthority()) return;

    bIsHost = bNewHost;
    ForceNetUpdate();
}

void AMyPlayerState::SetLobbyJoinOrder_Server(int32 NewJoinOrder)
{
    if (!HasAuthority()) return;

    LobbyJoinOrder = FMath::Max(0, NewJoinOrder);
    ForceNetUpdate();
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
    MyPS->LobbyJoinOrder = LobbyJoinOrder;
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
    LobbyJoinOrder = MyPS->LobbyJoinOrder;
}

void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMyPlayerState, Nickname);
    DOREPLIFETIME(AMyPlayerState, Preference);
    DOREPLIFETIME(AMyPlayerState, FinalRole);
    DOREPLIFETIME(AMyPlayerState, bIsHost);
    DOREPLIFETIME(AMyPlayerState, LobbyJoinOrder);
    DOREPLIFETIME(AMyPlayerState, CollectedClueNumbers);
    DOREPLIFETIME_CONDITION(AMyPlayerState, PositionedClueNumbers, COND_OwnerOnly);
}

bool AMyPlayerState::HasCollectedClue(int32 InClueNumber) const
{
    return CollectedClueNumbers.Contains(InClueNumber);
}

bool AMyPlayerState::CollectClue_Server(int32 InClueNumber)
{
    if (!HasAuthority()) return false;
    if (InClueNumber < 0 || InClueNumber > 9) return false;

    if (CollectedClueNumbers.Contains(InClueNumber))
    {
        return false;
    }

    CollectedClueNumbers.Add(InClueNumber);

    if (const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr)
    {
        GS->BuildPositionedClueNumbers(CollectedClueNumbers, PositionedClueNumbers);
    }

    ForceNetUpdate();
    NotifyClueCollectionChanged();
    return true;
}

void AMyPlayerState::OnRep_CollectedClueNumbers()
{
    NotifyClueCollectionChanged();
}

void AMyPlayerState::OnRep_PositionedClueNumbers()
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

    MyPC->UpdateGameRoomClues(PositionedClueNumbers);
}

