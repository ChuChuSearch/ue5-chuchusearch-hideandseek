#include "ConnectionNoticeSubsystem.h"

#include "ConnectionNoticeWidget.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"

void UConnectionNoticeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (GEngine)
    {
        NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UConnectionNoticeSubsystem::HandleNetworkFailure);
        TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UConnectionNoticeSubsystem::HandleTravelFailure);
    }
    PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UConnectionNoticeSubsystem::HandlePostLoadMap);
}

void UConnectionNoticeSubsystem::Deinitialize()
{
    if (GEngine)
    {
        GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
        GEngine->OnTravelFailure().Remove(TravelFailureHandle);
    }
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

    Super::Deinitialize();
}

bool UConnectionNoticeSubsystem::IsMyWorld(const UWorld* World) const
{
    // PIE에서는 창마다 GameInstance가 따로라서, 내 창의 일인지 확인한다
    return World && World->GetGameInstance() == GetGameInstance();
}

void UConnectionNoticeSubsystem::SetPending(const FString& Message)
{
    PendingMessage = Message;
    UE_LOG(LogTemp, Log, TEXT("Connection notice queued: %s"), *Message);
}

void UConnectionNoticeSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
    // 클라이언트가 서버와의 연결을 잃은 경우만(서버 쪽 실패나 다른 창은 무시)
    if (!IsMyWorld(World) || World->GetNetMode() != NM_Client)
    {
        return;
    }
    if (NetDriver && NetDriver->NetDriverName != NAME_GameNetDriver && NetDriver->NetDriverName != NAME_PendingNetDriver)
    {
        return;
    }

    // 서버가 PreLogin에서 거절한 이유(WaitRoomGameMode / MyGameMode가 보낸 문구)
    if (ErrorString.Contains(TEXT("Server full")))
    {
        SetPending(TEXT("방이 가득 차서 들어갈 수 없어요."));
        return;
    }
    if (ErrorString.Contains(TEXT("Match in progress")))
    {
        SetPending(TEXT("이미 게임이 진행 중이에요.\n게임이 끝나면 다시 들어와 주세요."));
        return;
    }

    switch (FailureType)
    {
    case ENetworkFailure::ConnectionLost:
    case ENetworkFailure::ConnectionTimeout:
        SetPending(TEXT("방장과의 연결이 끊어졌어요.\n방장이 게임을 나갔거나 인터넷 연결이 불안정해요."));
        break;
    default:
        SetPending(TEXT("서버와 연결할 수 없어요.\n주소를 확인하고 다시 시도해 주세요."));
        break;
    }
}

void UConnectionNoticeSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
    if (World && !IsMyWorld(World))
    {
        return;
    }

    SetPending(TEXT("서버에 들어갈 수 없어요.\n잠시 뒤 다시 시도해 주세요."));
}

void UConnectionNoticeSubsystem::HandlePostLoadMap(UWorld* World)
{
    if (PendingMessage.IsEmpty() || !IsMyWorld(World))
    {
        return;
    }

    // 접속 화면 UI 위에 팝업을 띄운다
    if (UConnectionNoticeWidget* Popup = CreateWidget<UConnectionNoticeWidget>(GetGameInstance(), UConnectionNoticeWidget::StaticClass()))
    {
        Popup->SetMessage(PendingMessage);
        Popup->AddToViewport(5000);
    }
    PendingMessage.Empty();
}
