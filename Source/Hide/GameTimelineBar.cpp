#include "GameTimelineBar.h"

#include "Engine/World.h"
#include "Rendering/DrawElements.h"

namespace
{
    FLinearColor Hex(uint8 R, uint8 G, uint8 B, uint8 A = 255)
    {
        return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
    }

    // 지나지 않은 구간은 이만큼만 보이게(어떤 단계가 남았는지 알 수 있을 정도)
    constexpr float UpcomingOpacity = 0.28f;
}

void UGameTimelineBar::NativeConstruct()
{
    Super::NativeConstruct();
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

FLinearColor UGameTimelineBar::PhaseColor(EGamePhase Phase)
{
    switch (Phase)
    {
    case EGamePhase::HideTime:   return Hex(96, 196, 255);   // 하늘색: 숨는 시간
    case EGamePhase::Phase1:     return Hex(255, 200, 64);   // 금색: 1단계
    case EGamePhase::ForcedSwap: return Hex(255, 128, 40);   // 주황: 강제 교체
    case EGamePhase::Phase2:     return Hex(255, 214, 96);   // 밝은 금색: 2단계
    case EGamePhase::FeverTime:  return Hex(255, 64, 72);    // 빨강: 피버
    default:                     return Hex(200, 200, 200);
    }
}

void UGameTimelineBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    PulseTime += InDeltaTime;

    const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    if (!GS)
    {
        return;
    }

    Timeline = GS->GetPhaseTimeline();
    TotalSeconds = 0.f;
    for (const FPhaseTimelineEntry& Entry : Timeline)
    {
        TotalSeconds += Entry.DurationSeconds;
    }

    const EGamePhase Phase = GS->GetGamePhase();

    // 게임이 끝나면 끝난 순간의 위치에 멈춘다(시간 만료면 끝까지 찬 상태)
    if (Phase == EGamePhase::Ended)
    {
        bEnded = true;
        return;
    }
    bEnded = false;

    CurrentIndex = Timeline.IndexOfByPredicate([Phase](const FPhaseTimelineEntry& E) { return E.Phase == Phase; });
    if (CurrentIndex == INDEX_NONE)
    {
        ElapsedSeconds = 0.f;
        return;
    }

    float Before = 0.f;
    for (int32 i = 0; i < CurrentIndex; ++i)
    {
        Before += Timeline[i].DurationSeconds;
    }

    const float Duration = Timeline[CurrentIndex].DurationSeconds;
    const float Remaining = static_cast<float>(GS->GetPhaseRemainingSeconds());
    ElapsedSeconds = Before + FMath::Clamp(Duration - Remaining, 0.f, Duration);
}

int32 UGameTimelineBar::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

    const FVector2f Size = AllottedGeometry.GetLocalSize();
    if (TotalSeconds <= KINDA_SMALL_NUMBER || Size.X <= 1.f || Size.Y <= 1.f)
    {
        return LayerId;
    }

    const int32 BaseLayer = LayerId + 1;
    const int32 FillLayer = LayerId + 2;
    const int32 MarkLayer = LayerId + 3;

    auto DrawRect = [&](int32 Layer, float X, float Y, float W, float H, const FLinearColor& Color)
    {
        if (W <= 0.f || H <= 0.f)
        {
            return;
        }
        FSlateDrawElement::MakeBox(
            OutDrawElements, Layer,
            AllottedGeometry.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y))),
            &SolidBrush, ESlateDrawEffect::None, Color);
    };

    const float FillX = Size.X * FMath::Clamp(ElapsedSeconds / TotalSeconds, 0.f, 1.f);
    const float HighlightH = Size.Y * 0.35f;

    // 이 위젯은 ScaleBox 안 프레임 이미지 크기 기준이라 크게 줄어 보인다. 선 두께는 화면 픽셀 기준으로 맞춘다.
    const float Px = 1.f / FMath::Max(AllottedGeometry.Scale, KINDA_SMALL_NUMBER);

    float X = 0.f;
    for (int32 i = 0; i < Timeline.Num(); ++i)
    {
        const float W = Size.X * (Timeline[i].DurationSeconds / TotalSeconds);
        FLinearColor Color = PhaseColor(Timeline[i].Phase);

        // 남은 구간은 옅게 깔아 둔다
        DrawRect(BaseLayer, X, 0.f, W, Size.Y, Color.CopyWithNewOpacity(UpcomingOpacity));

        // 지난 부분은 진하게 + 위쪽에 밝은 줄(입체감)
        const float FilledW = FMath::Clamp(FillX - X, 0.f, W);
        if (FilledW > 0.f)
        {
            // 진행 중인 구간은 살짝 숨 쉬듯 밝아졌다 어두워진다(피버는 더 빠르게)
            if (i == CurrentIndex && !bEnded)
            {
                const float Speed = Timeline[i].Phase == EGamePhase::FeverTime ? 7.f : 3.f;
                const float Pulse = 0.85f + 0.15f * FMath::Sin(PulseTime * Speed);
                Color = FLinearColor(Color.R * Pulse, Color.G * Pulse, Color.B * Pulse, 1.f);
            }

            DrawRect(FillLayer, X, 0.f, FilledW, Size.Y, Color);
            DrawRect(FillLayer, X, 0.f, FilledW, HighlightH, FLinearColor(1.f, 1.f, 1.f, 0.22f));
        }

        // 구간 경계 눈금(맨 앞 제외)
        if (i > 0)
        {
            DrawRect(MarkLayer, X - 1.f * Px, 0.f, 2.f * Px, Size.Y, Hex(20, 18, 40, 200));
        }

        X += W;
    }

    // 지금 위치 표시(흰 세로 선 + 옅은 빛)
    if (!bEnded && FillX > 0.f && FillX < Size.X)
    {
        DrawRect(MarkLayer, FillX - 4.f * Px, 0.f, 8.f * Px, Size.Y, FLinearColor(1.f, 1.f, 1.f, 0.25f));
        DrawRect(MarkLayer, FillX - 1.5f * Px, 0.f, 3.f * Px, Size.Y, FLinearColor::White);
    }

    return MarkLayer;
}
