#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyPlayerState.h"
#include "RoleChoiceButtonWidget.generated.h"

class UButton;
class UTextBlock;

// 대기실 역할 선택 버튼(술래 지망 / 러너 지망 / 상관없음). 세 버튼이 같은 모양이다.
// 누르면 지망을 바꾸고, 지금 고른 지망이면 역할 색으로 채워진 선택 모양으로 보인다.
UCLASS()
class HIDE_API URoleChoiceButtonWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 화면에 넣기 전에 호출
    void Setup(ERolePreference InPreference, const FString& InLabel, const FLinearColor& InSelectedFill, const FLinearColor& InSelectedLine, int32 InFontSize);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildTree();
    void ApplyStyle(bool bSelected);

    UFUNCTION()
    void OnClicked();

    ERolePreference Preference = ERolePreference::Any;
    FString LabelText = TEXT("상관없음");
    FLinearColor SelectedFill = FLinearColor::White;
    FLinearColor SelectedLine = FLinearColor::White;
    int32 FontSize = 22;

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
