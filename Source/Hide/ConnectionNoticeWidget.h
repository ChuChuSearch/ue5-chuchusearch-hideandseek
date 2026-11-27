#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConnectionNoticeWidget.generated.h"

class UTextBlock;

// 연결이 끊겨 접속 화면으로 돌아왔을 때 이유를 알려 주는 팝업(확인을 누르면 닫힌다).
// 설정 창과 같은 크림색 패널 + 리본 제목.
UCLASS()
class HIDE_API UConnectionNoticeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetMessage(const FString& InMessage);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    void BuildTree();

    UFUNCTION()
    void OnConfirmClicked();

    FString Message;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> MessageText;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> LightFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> ClickSound;
};
