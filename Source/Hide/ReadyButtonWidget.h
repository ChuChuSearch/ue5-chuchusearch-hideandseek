#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReadyButtonWidget.generated.h"

class UButton;
class UTextBlock;
class UTexture2D;

// 대기실 "시작하기 / 준비하기" 버튼. START 버튼 그림에서 글자만 지운 READY_Button에 한글 글자를 얹고, START 자리에 둔다.
// 원래 START(영어 글자 그림)는 숨기고, 방장에게는 "시작하기", 나머지에게는 "준비하기"로 보인다.
UCLASS()
class HIDE_API UReadyButtonWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UReadyButtonWidget(const FObjectInitializer& ObjectInitializer);

    // 대신할 원래 START 버튼(WBP의 ScaleBox_StartButton). 항상 숨긴다.
    void SetStartWidget(UWidget* InStartWidget) { StartWidget = InStartWidget; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildTree();
    // bHost면 "시작하기"(bActive = 시작 가능), 아니면 "준비하기"(bActive = 준비함)
    void ApplyState(bool bHost, bool bActive);

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
