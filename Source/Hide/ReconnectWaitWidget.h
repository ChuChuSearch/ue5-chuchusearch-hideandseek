#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReconnectWaitWidget.generated.h"

class UBorder;
class UTextBlock;

// 한 팀이 모두 끊겼을 때 화면 위쪽에 띄우는 카운트다운 띠.
// "술래가 모두 나갔어요 · 0:45 안에 돌아오지 않으면 숨는 팀 승리"
// GameState의 재접속 대기 정보(AMyGameState::GetReconnectWaitRole)를 매 틱 읽어 보여 주고, 대기가 끝나면 숨는다.
UCLASS()
class HIDE_API UReconnectWaitWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildTree();

    UPROPERTY(Transient)
    TObjectPtr<UBorder> Banner;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DetailText;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    int32 LastShownSeconds = -1;
    int32 LastRole = -1;
};
