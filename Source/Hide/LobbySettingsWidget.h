#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyGameSettings.h"
#include "LobbySettingsWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class ULobbySettingsWidget;

// < > 버튼 하나가 "어느 항목을 몇 칸 옮길지" 기억한다(UButton::OnClicked에는 어떤 버튼인지가 없어서)
UCLASS()
class HIDE_API ULobbySettingStepHandler : public UObject
{
    GENERATED_BODY()

public:
    TWeakObjectPtr<ULobbySettingsWidget> Owner;
    ELobbySettingField Field = ELobbySettingField::MaxPlayers;
    int32 Step = 0;

    UFUNCTION()
    void HandleClicked();
};

// 대기실 "게임 설정" 창. 방장은 < >로 값을 바꾸고, 다른 사람은 같은 창에서 보기만 한다.
// 값은 서버가 보내 준 설정(AMyPlayerController::GetLobbyGameSettings)을 그대로 보여 준다.
UCLASS()
class HIDE_API ULobbySettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void StepField(ELobbySettingField Field, int32 Step);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    struct FRowWidgets
    {
        UButton* Prev = nullptr;
        UButton* Next = nullptr;
        UTextBlock* Value = nullptr;
    };

    void BuildTree();
    void AddRow(UVerticalBox* Box, ELobbySettingField Field);
    UButton* MakeArrowButton(const FString& Label, ELobbySettingField Field, int32 Step);
    UButton* MakeTextButton(const FString& Label, const FLinearColor& Fill, const FLinearColor& Line, const FLinearColor& TextColor);
    void Refresh();

    UFUNCTION()
    void OnResetClicked();

    UFUNCTION()
    void OnCloseClicked();

    FSlateFontInfo MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor = FLinearColor::Transparent, int32 OutlineSize = 0) const;

    TArray<FRowWidgets> Rows;

    UPROPERTY(Transient)
    TArray<TObjectPtr<ULobbySettingStepHandler>> StepHandlers;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TotalText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> NoteText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ResetButton;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> LightFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> ClickSound;

    UPROPERTY(Transient)
    TObjectPtr<UObject> HoverSound;

    // 화면에 그려 둔 설정 / 방장 여부(바뀔 때만 다시 쓴다)
    FLobbyGameSettings ShownSettings;
    int32 ShownHost = -1;
    int32 ShownPlayerCount = -1;
    bool bShownAny = false;
};
