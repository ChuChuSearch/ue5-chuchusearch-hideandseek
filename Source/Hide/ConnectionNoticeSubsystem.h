#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/EngineBaseTypes.h"
#include "ConnectionNoticeSubsystem.generated.h"

class UNetDriver;

// 서버(방장)와의 연결이 끊기거나 접속이 거절되면 엔진이 접속 화면(ConnectMap)으로 돌려보낸다.
// 그때 왜 돌아왔는지 모르기 쉬우므로, 이유를 기억해 두었다가 다음 맵이 열리면 팝업으로 알려 준다.
UCLASS()
class HIDE_API UConnectionNoticeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
    void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
    void HandlePostLoadMap(UWorld* World);

    bool IsMyWorld(const UWorld* World) const;
    void SetPending(const FString& Message);

    FString PendingMessage;

    FDelegateHandle NetworkFailureHandle;
    FDelegateHandle TravelFailureHandle;
    FDelegateHandle PostLoadMapHandle;
};
