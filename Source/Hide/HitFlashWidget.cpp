#include "HitFlashWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"

namespace
{
    // 바깥쪽이 가장 진하고 안쪽으로 갈수록 옅어지는 겹 수/두께
    constexpr int32 LayerCount = 8;
    constexpr float LayerWidth = 14.f;
}

TSharedRef<SWidget> UHitFlashWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

void UHitFlashWidget::BuildTree()
{
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
    WidgetTree->RootWidget = Root;

    for (int32 i = 0; i < LayerCount; ++i)
    {
        const float T = static_cast<float>(i) / LayerCount;         // 0(바깥) ~ 1(안쪽)
        const float Alpha = 0.55f * FMath::Square(1.f - T);
        const float Radius = 8.f + i * LayerWidth * 1.5f;            // 안쪽일수록 둥글게

        FSlateRoundedBoxBrush Brush(FLinearColor::Transparent, FLinearColor(0.85f, 0.f, 0.05f, Alpha), LayerWidth);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;

        UBorder* Layer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Layer->SetBrush(Brush);
        if (UOverlaySlot* S = Root->AddChildToOverlay(Layer))
        {
            S->SetPadding(FMargin(i * LayerWidth * 0.75f));
            S->SetHorizontalAlignment(HAlign_Fill);
            S->SetVerticalAlignment(VAlign_Fill);
        }
    }

    SetVisibility(ESlateVisibility::Collapsed);
}

void UHitFlashWidget::Play(float Intensity, float Duration)
{
    StartIntensity = FMath::Clamp(Intensity, 0.f, 1.f);
    FadeDuration = FMath::Max(0.05f, Duration);
    Elapsed = 0.f;
    bPlaying = true;

    SetRenderOpacity(StartIntensity);
    // 마우스 클릭을 막지 않도록
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHitFlashWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bPlaying)
    {
        return;
    }

    Elapsed += InDeltaTime;
    const float Remain = 1.f - FMath::Clamp(Elapsed / FadeDuration, 0.f, 1.f);
    SetRenderOpacity(StartIntensity * Remain * Remain);

    if (Remain <= 0.f)
    {
        bPlaying = false;
        SetVisibility(ESlateVisibility::Collapsed);
    }
}
