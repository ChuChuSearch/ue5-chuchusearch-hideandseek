#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReadyButtonWidget.generated.h"

class UButton;
class UTextBlock;
class UTexture2D;

// 대기실 "준비하기" 버튼. START 버튼과 같은 그림(글자만 지운 READY_Button)에 글자를 얹고, START와 같은 자리에 둔다.
// 방장이면 이 버튼을 숨기고 원래 START 버튼을 보이게, 방장이 아니면 START를 숨기고 이 버튼을 보이게 바꾼다.
UCLASS()
class HIDE_API UReadyButtonWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UReadyButtonWidget(const FObjectInitializer& ObjectInitializer);

    // 방장 여부에 따라 서로 바꿔 보여 줄 START 버튼(WBP의 ScaleBox_StartButton)
    void SetStartWidget(UWidget* InStartWidget) { StartWidget = InStartWidget; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildTree();
    void ApplyState(bool bReady);

    UFUNCTION()
    void OnClicked();

    UPROPERTY(Transient)
    TObjectPtr<UButton> Button;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> Label;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> NormalTexture;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> HoverTexture;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> PushTexture;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> ClickSound;

    UPROPERTY(Transient)
    TObjectPtr<UObject> HoverSound;

    TWeakObjectPtr<UWidget> StartWidget;

    int32 LastHost = -1;
    int32 LastReady = -1;
};
