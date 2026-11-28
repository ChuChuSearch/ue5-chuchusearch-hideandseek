#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyPlayerState.h"
#include "LobbyGameSettings.h"
#include "WaitRoomGameMode.generated.h"

class AMyPlayerState;
class APlayerController;

UCLASS()
class HIDE_API AWaitRoomGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AWaitRoomGameMode();

    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual void HandleSeamlessTravelPlayer(AController*& C) override;

    // 로비 맵에서 돌아다닐 캐릭터. LobbyPawnClass가 비어 있으면 게임 모드(BP_MyGameMode)의 캐릭터를 쓴다.
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

    UFUNCTION(BlueprintCallable)
    void StartGame(APlayerController* RequestPC);

    // 시작 조건(방장, 인원 수 등)을 통과해 실제로 이동을 시작했으면 true.
    // 시작하지 못하면 OutFailReason에 이유(방장에게 채팅 안내로 보여 줌)
    bool TryStartGame(APlayerController* RequestPC, FString* OutFailReason = nullptr);

    // 지금 시작할 수 있는지만 확인(방장 / 2명 이상 / 최대 인원 / 손님 모두 준비). 출발 발판 카운트다운에서 쓴다.
    bool CheckStartConditions(APlayerController* RequestPC, FString* OutFailReason = nullptr) const;

    // ===== 대기실 채팅 =====
    // 플레이어가 보낸 채팅(서버에서 다듬고 너무 잦으면 버린다)
    void HandleLobbyChat(APlayerController* SenderPC, const FString& RawMessage);

    // 모든 플레이어에게 채팅 한 줄 보내기(bSystem이면 "누가 들어왔어요" 같은 안내)
    void BroadcastLobbyChat(const FString& SenderName, const FString& Message, bool bSystem);

    // ===== 방장 게임 설정 =====
    // 지금 적용 중인 설정(서버 GameInstance에 저장돼 맵이 바뀌어도 유지)
    FLobbyGameSettings GetLobbyGameSettings() const;

    // 방장이 보낸 새 설정. 방장이 아니거나 지금 인원보다 최대 인원이 작으면 무시/조정한다.
    void HandleSetLobbyGameSettings(APlayerController* RequestPC, const FLobbyGameSettings& NewSettings, bool bResetToDefaults);

    int32 GetMaxLobbyPlayers() const;

    // 닉네임이 정해지면(입력이 잠깐 멈추면) 한 번만 입장 안내를 보낸다
    void NotifyNicknameSet(AMyPlayerState* PS);

    // ===== 로비 연습 =====
    // 술래 지망이면 술래로, 아니면 러너로 연습한다(역할 문으로 지망을 바꿀 때, 캐릭터가 생길 때)
    void ApplyPracticeRole(APawn* Pawn);

    // 부서진 물체를 잠시 뒤 같은 자리에 다시 놓는다
    void ScheduleLobbyPropRespawn(TSubclassOf<AActor> PropClass, const FTransform& Transform);

    // 유령 구멍: 사람이면 유령으로, 유령이면 다시 사람으로
    void ToggleLobbyGhost(APlayerController* PC, const AActor* Hole);

    virtual void SetPlayerDefaults(APawn* PlayerPawn) override;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Practice", meta = (ClampMin = "0.5"))
    float LobbyPropRespawnSeconds = 5.f;

    // 유령 체험에서 놓을 물체 후보를 항상 이만큼 채워 둔다(사진이 있는 게임 물체 중에서 무작위)
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Practice", meta = (ClampMin = "1"))
    int32 LobbyGhostPoolSize = 9;

    // 게임 맵에 놓인 물체 크기(게임 물체는 맵에서 이 배율로 놓여 있다)
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Practice", meta = (ClampMin = "0.01"))
    float LobbyGhostPropScale = 25.4f;

    // 유령이 로비에 놓은 물체가 이보다 많아지면 오래된 것부터 치운다
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Practice", meta = (ClampMin = "1"))
    int32 MaxLobbyGhostPlacedProps = 12;

    // 유령이 로비에 물체를 놓았다(개수 제한용)
    void NotifyLobbyGhostPlacedProp(AActor* Prop);

    virtual void BeginPlay() override;

    // 같은 사람이 이보다 자주 보내면 버린다(초)
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Chat", meta = (ClampMin = "0.0"))
    float ChatMinIntervalSeconds = 0.5f;

    static constexpr int32 MaxChatLength = 80;

protected:
    // 대기실 최대 인원의 처음 값. 방장이 게임 설정에서 바꿀 수 있다(초과 접속은 PreLogin에서 거부).
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 MaxLobbyPlayers = 4;

    // 로비 맵 캐릭터(비우면 게임 맵 게임 모드의 PlayerCharacterClass)
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    TSubclassOf<APawn> LobbyPawnClass;

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

    // 게임 맵에서 쓸 게임 모드 클래스(이동 주소의 ?game=)
    UClass* LoadGameModeClassFromTravelURL() const;

    // 게임 맵으로 떠나기 전에 로비 캐릭터를 치운다
    void DestroyLobbyPawns();

    // 물체를 다시 놓는다. 그 자리에 사람이 있으면 조금 뒤 다시 시도.
    void TryRespawnLobbyProp(TSubclassOf<AActor> PropClass, FTransform Transform);

    // 유령 체험 물체 후보 채우기
    void TopUpLobbyGhostPool();
    FTimerHandle TH_LobbyGhostPool;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UClass>> LobbyGhostPropClasses;
    bool bLobbyGhostClassesLoaded = false;

    TArray<TWeakObjectPtr<AActor>> LobbyGhostPlacedProps;

    int32 NextLobbyJoinOrder = 1;

    // 플레이어별 마지막 채팅 시각(도배 방지)
    TMap<TWeakObjectPtr<APlayerController>, double> LastChatTime;

    // 입장 안내: 닉네임 입력 중에는 계속 미루다가 멈추면 보낸다
    TMap<TWeakObjectPtr<AMyPlayerState>, FTimerHandle> JoinAnnounceTimers;
    TSet<TWeakObjectPtr<AMyPlayerState>> AnnouncedPlayers;
    void AnnounceJoin(TWeakObjectPtr<AMyPlayerState> PS);

    class ULobbySettingsSubsystem* GetSettingsSubsystem() const;

    // 처음 한 번, 게임 모드 블루프린트(BP_MyGameMode, 이 대기실 모드)의 값으로 설정 기본값을 채운다
    void InitializeLobbySettingsDefaults();

    // 모든 플레이어에게 지금 설정을 보낸다
    void BroadcastLobbyGameSettings();
    void SendLobbyGameSettings(APlayerController* PC) const;

    // 설정이 바뀌었다는 채팅 안내(연달아 바꾸면 마지막에 한 번만)
    FTimerHandle TH_SettingsChangedNotice;
    void AnnounceSettingsChanged();
};
