#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyPanelWidget.generated.h"

class AMyPlayerState;
class UBorder;
class UButton;
class UEditableTextBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

// 대기실 화면 오른쪽 패널: 위는 참가자 목록(닉네임, 방장, 술래/러너 지망), 아래는 채팅.
// WBP_WaitRoom 위에 따로 띄운다. WBP 없이 C++에서 그리고, 설정 창과 같은 크림색 패널 + 리본 제목 톤.
UCLASS()
class HIDE_API ULobbyPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 채팅 한 줄 추가(bSystem이면 "누구님이 들어왔어요" 같은 회색 안내)
    void AddChatLine(const FString& SenderName, const FString& Message, bool bSystem);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildTree();
    UWidget* MakePanel(const FString& Title, UVerticalBox*& OutBody, UTextBlock** OutTitleText = nullptr);

    void RefreshPlayers();
    void AddPlayerRow(const AMyPlayerState* PS, bool bLocal);
    UWidget* MakeChip(const FString& Text, const FLinearColor& Fill, const FLinearColor& TextColor, int32 FontSize);

    void UpdateReadyArea(const TArray<const AMyPlayerState*>& Players, const AMyPlayerState* LocalPS);

    void SubmitChat();

    UFUNCTION()
    void OnChatCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnChatChanged(const FText& Text);

    UFUNCTION()
    void OnSendClicked();

    FSlateFontInfo MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor = FLinearColor::Transparent, int32 OutlineSize = 0) const;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> PlayerRows;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PlayerTitle;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PlayersHint;

    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> ChatScroll;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> ChatInput;

    UPROPERTY(Transient)
    TObjectPtr<UButton> SendButton;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> LightFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> ClickSound;

    // 참가자 목록이 바뀌었을 때만 다시 그린다(매번 다시 만들면 깜빡인다)
    FString LastPlayersSignature;
    float RefreshRemain = 0.f;

    bool bScrollToEndPending = false;
};
