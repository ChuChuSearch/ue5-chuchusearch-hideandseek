#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/SlateBrush.h"
#include "CrosshairWidget.generated.h"

// 화면 정중앙 조준점. 이미지 대신 직접 그린다(가운데 점 + 네 방향 짧은 선, 어두운 테두리).
// - 조준한 곳에 물체나 다른 플레이어가 있으면 코랄색으로 바뀐다.
// - 술래가 쏘면 잠깐 벌어졌다가 돌아온다. 재사용 대기 중에는 흐리게 보인다.
UCLASS()
class HIDE_API UCrosshairWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UCrosshairWidget(const FObjectInitializer& ObjectInitializer);

    // 발사했을 때 호출(벌어지는 효과)
    void NotifyFired();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    void UpdateState(float DeltaTime);

    FSlateColorBrush SolidBrush = FSlateColorBrush(FLinearColor::White);
    FSlateBrush DotBrush;
    FSlateBrush DotOutlineBrush;

    // 지금 보여 줄지(살아 있는 캐릭터일 때만)
    bool bShow = false;

    // 조준한 곳에 대상이 있는지
    bool bOnTarget = false;

    // 술래 재사용 대기 중인지
    bool bCoolingDown = false;

    // 발사 직후 벌어진 정도(0~1), 색 전환 정도(0~1)
    float Kick = 0.f;
    float TargetBlend = 0.f;
};
