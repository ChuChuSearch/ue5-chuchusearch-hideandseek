#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Brushes/SlateColorBrush.h"
#include "MyGameState.h"
#include "GameTimelineBar.generated.h"

// 게임 전체 타임라인 바(WBP_GameRoom 위쪽 프레임 아래 빈 홈에 끼워 넣는다).
// GameMode의 단계 시간(GameState::PhaseTimeline)을 비율대로 나눠 단계 색으로 그리고,
// 지난 시간만큼 밝게 채운 뒤 지금 위치에 표시를 둔다. 단계 시간이 바뀌면 자동으로 맞춰진다.
UCLASS()
class HIDE_API UGameTimelineBar : public UUserWidget
{
    GENERATED_BODY()

public:
    // 프레임 이미지(TimeBackgrouind) 안에서 빈 홈이 차지하는 비율(이미지 픽셀을 재서 구함)
    static constexpr float GrooveLeft = 664.f / 3683.f;
    static constexpr float GrooveRight = 3002.f / 3683.f;
    static constexpr float GrooveTop = 876.f / 1192.f;
    static constexpr float GrooveBottom = 929.f / 1192.f;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    static FLinearColor PhaseColor(EGamePhase Phase);

    FSlateColorBrush SolidBrush = FSlateColorBrush(FLinearColor::White);

    // 그릴 때 쓰는 값(틱에서 갱신)
    TArray<FPhaseTimelineEntry> Timeline;
    float TotalSeconds = 0.f;
    float ElapsedSeconds = 0.f;
    int32 CurrentIndex = INDEX_NONE;
    float PulseTime = 0.f;
    bool bEnded = false;
};
