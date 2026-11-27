#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AnyRoleButtonWidget.generated.h"

class UButton;
class UTextBlock;

// 대기실 역할 선택의 "상관없음" 버튼(술래 / 러너 버튼 아래).
// 누르면 지망을 '상관없음'으로 바꾸고, 지금 '상관없음'이면 선택된 모양(코랄)으로 보인다.
UCLASS()
class HIDE_API UAnyRoleButtonWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildTree();
    void ApplyStyle(bool bSelected);

    UFUNCTION()
    void OnClicked();

    UPROPERTY(Transient)
    TObjectPtr<UButton> Button;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> Label;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> ClickSound;

    UPROPERTY(Transient)
    TObjectPtr<UObject> HoverSound;

    int32 LastSelected = -1;
};
