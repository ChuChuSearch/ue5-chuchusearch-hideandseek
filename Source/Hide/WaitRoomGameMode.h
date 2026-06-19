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

    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

    UFUNCTION(BlueprintCallable)
    void StartGame(APlayerController* RequestPC);

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Travel")
    TSoftObjectPtr<UWorld> GameMapAsset;

    UPROPERTY(EditDefaultsOnly, Category = "Travel")
    FString GameMapTravelURL =
        TEXT("/Game/Game/Levels/GameMap?game=/Game/Game/Blueprints/GameMode/BP_MyGameMode.BP_MyGameMode_C");

private:
    void EnsureHost();
    bool PickSeeker(AMyPlayerState*& OutSeekerPS) const;

    FString BuildGameTravelURL() const;
};
