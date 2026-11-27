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
    if (!IsInWaitRoom_Server())
    {
        UE_LOG(LogTemp, Warning, TEXT("ServerSetNickname rejected (not in wait room): '%s' for %s"), *InNickname, *GetPlayerName());
        return;
    }

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
        UE_LOG(LogTemp, Warning, TEXT("ServerSetNickname rejected (empty): %s"), *GetPlayerName());
        return;
    }

    Nickname = Sanitized;
    UE_LOG(LogTemp, Log, TEXT("ServerSetNickname: %s -> '%s'"), *GetPlayerName(), *Nickname);
    ForceNetUpdate();

    // 대기실 채팅에 "누구님이 들어왔어요" 안내(닉네임 입력이 멈춘 뒤 한 번)
    if (AWaitRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWaitRoomGameMode>() : nullptr)
    {
        GM->NotifyNicknameSet(this);
    }
}

void AMyPlayerState::ServerSetReady_Implementation(bool bNewReady)
{
    if (!HasAuthority() || !IsInWaitRoom_Server() || bIsHost)
    {
        return;
    }

    if (bReady != bNewReady)
    {
        bReady = bNewReady;
        ForceNetUpdate();
    }
}

void AMyPlayerState::ResetReady_Server()
{
    if (HasAuthority() && bReady)
    {
        bReady = false;
        ForceNetUpdate();
    }
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
    OutData.ScoreStats = ScoreStats;
    OutData.bRoundEliminated = bRoundEliminated;
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
    ScoreStats = InData.ScoreStats;
    bRoundEliminated = InData.bRoundEliminated;

    ForceNetUpdate();
    NotifyClueCollectionChanged();
}

void AMyPlayerState::SetHost_Server(bool bNewHost)
{
    if (!HasAuthority()) return;

    bIsHost = bNewHost;

    // 방장은 준비 대상이 아니다
    if (bIsHost)
    {
        bReady = false;
    }
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
    DOREPLIFETIME_CONDITION(AMyPlayerState, ScoreStats, COND_OwnerOnly);
    DOREPLIFETIME(AMyPlayerState, RevealedScoreStats);
    DOREPLIFETIME(AMyPlayerState, bScoresRevealed);
    DOREPLIFETIME(AMyPlayerState, bRoundEliminated);
    DOREPLIFETIME(AMyPlayerState, bReady);
}

void AMyPlayerState::RevealScores_Server()
{
    if (!HasAuthority())
    {
        return;
    }

    RevealedScoreStats = ScoreStats;
    bScoresRevealed = true;
    ForceNetUpdate();
}

void AMyPlayerState::MarkEliminated_Server()
{
    if (!HasAuthority() || bRoundEliminated)
    {
        return;
    }

    bRoundEliminated = true;
    ForceNetUpdate();
}

bool AMyPlayerState::AddScore_Server(EScoreReason Reason)
{
    if (!HasAuthority() || IsInWaitRoom_Server() || bScoresRevealed)
    {
        return false;
    }

    int32 Points = 0;
    switch (Reason)
    {
    case EScoreReason::SeekerHit:
        ++ScoreStats.Hits;
        Points = ScoreSeekerHit;
        break;

    case EScoreReason::SeekerEliminate:
        ++ScoreStats.Eliminations;
        Points = ScoreSeekerEliminate;
        break;

    case EScoreReason::ClueCollected:
        ++ScoreStats.Clues;
        Points = ScoreClue;
        break;

    case EScoreReason::PasswordSolved:
        if (ScoreStats.bPasswordSolved)
        {
            return false;
        }
        ScoreStats.bPasswordSolved = true;
        Points = ScorePassword;
        break;

    case EScoreReason::CloseHide:
        Points = FMath::Min(ScoreCloseHidePerSecond, MaxCloseHidePoints - ScoreStats.CloseHidePoints);
        if (Points <= 0)
        {
            return false;
        }
        ScoreStats.CloseHidePoints += Points;
        break;

    case EScoreReason::NearMiss:
        if (ScoreStats.NearMisses >= MaxNearMisses)
        {
            return false;
        }
        ++ScoreStats.NearMisses;
        Points = ScoreNearMiss;
        break;

    case EScoreReason::DecoyHit:
        if (ScoreStats.DecoyHits >= MaxDecoyHits)
        {
            return false;
        }
        ++ScoreStats.DecoyHits;
        Points = ScoreDecoyHit;
        break;

    case EScoreReason::Win:
        Points = ScoreWin;
        break;

    default:
        return false;
    }

    ScoreStats.Score += Points;
    ForceNetUpdate();
    return true;
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
    AddScore_Server(EScoreReason::ClueCollected);

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

