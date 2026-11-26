#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnCountdownWidget.generated.h"

class UButton;
class UTextBlock;

// 결과 화면 아래쪽에 "N초 후 대기실로 돌아갑니다"와 방장 전용 "바로 대기실로" 버튼을 보여준다.
// 남은 시간은 복제된 GameState의 종료 페이즈 남은 시간을 그대로 읽는다.
// WBP 없이 C++에서 그린다.
UCLASS()
class HIDE_API UReturnCountdownWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildTree();
    void Refresh();
    FSlateFontInfo MakeFont(bool bBold, int32 Size) const;

    UFUNCTION()
    void OnReturnNowClicked();

    int32 LastShownSecond = -2;
    bool bRequested = false;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CountdownText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ReturnNowButton;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ReturnNowText;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> LightFont;
};
