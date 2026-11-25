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
};
