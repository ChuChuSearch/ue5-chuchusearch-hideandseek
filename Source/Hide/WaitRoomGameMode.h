#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyPlayerState.h"
#include "WaitRoomGameMode.generated.h"

class AMyPlayerState;
class APlayerController;

UCLASS()
class HIDE_API AWaitRoomGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AWaitRoomGameMode();

    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual void HandleSeamlessTravelPlayer(AController*& C) override;

    UFUNCTION(BlueprintCallable)
    void StartGame(APlayerController* RequestPC);

    // 시작 조건(방장, 인원 수 등)을 통과해 실제로 이동을 시작했으면 true.
    bool TryStartGame(APlayerController* RequestPC);

    // ===== 대기실 채팅 =====
    // 플레이어가 보낸 채팅(서버에서 다듬고 너무 잦으면 버린다)
    void HandleLobbyChat(APlayerController* SenderPC, const FString& RawMessage);

    // 모든 플레이어에게 채팅 한 줄 보내기(bSystem이면 "누가 들어왔어요" 같은 안내)
    void BroadcastLobbyChat(const FString& SenderName, const FString& Message, bool bSystem);

    int32 GetMaxLobbyPlayers() const { return MaxLobbyPlayers; }

    // 닉네임이 정해지면(입력이 잠깐 멈추면) 한 번만 입장 안내를 보낸다
    void NotifyNicknameSet(AMyPlayerState* PS);

    // 같은 사람이 이보다 자주 보내면 버린다(초)
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Chat", meta = (ClampMin = "0.0"))
    float ChatMinIntervalSeconds = 0.5f;

    static constexpr int32 MaxChatLength = 80;

protected:
    // 대기실 최대 인원. 초과 접속은 PreLogin에서 거부하고, 게임 시작 조건에도 사용한다.
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 MaxLobbyPlayers = 4;

    UPROPERTY(EditDefaultsOnly, Category = "Travel")
    TSoftObjectPtr<UWorld> GameMapAsset;

    UPROPERTY(EditDefaultsOnly, Category = "Travel")
    FString GameMapTravelURL =
        TEXT("/Game/Game/Levels/GameMap?game=/Game/Game/Blueprints/Framework/GameMode/BP_MyGameMode.BP_MyGameMode_C");

private:
    void RegisterLobbyJoinOrder(AMyPlayerState* PlayerState);
    void EnsureHost(const AMyPlayerState* ExcludedPlayerState = nullptr);
    bool PickSeeker(AMyPlayerState*& OutSeekerPS) const;

    FString BuildGameTravelURL() const;

    int32 NextLobbyJoinOrder = 1;

    // 플레이어별 마지막 채팅 시각(도배 방지)
    TMap<TWeakObjectPtr<APlayerController>, double> LastChatTime;

    // 입장 안내: 닉네임 입력 중에는 계속 미루다가 멈추면 보낸다
    TMap<TWeakObjectPtr<AMyPlayerState>, FTimerHandle> JoinAnnounceTimers;
    TSet<TWeakObjectPtr<AMyPlayerState>> AnnouncedPlayers;
    void AnnounceJoin(TWeakObjectPtr<AMyPlayerState> PS);
};
