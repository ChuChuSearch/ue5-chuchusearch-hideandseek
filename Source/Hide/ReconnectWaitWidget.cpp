#include "ReconnectWaitWidget.h"

#include "MyGameState.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace
{
    const TCHAR* BoldFontPath = TEXT("/Game/Game/Fonts/Maplestory_Bold_Font.Maplestory_Bold_Font");

    FLinearColor Hex(uint8 R, uint8 G, uint8 B, uint8 A = 255)
    {
        return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
    }

    // 위쪽 팀 프레임 아래, 탈락 알림 위쪽
    constexpr float TopOffset = 140.f;
}

TSharedRef<SWidget> UReconnectWaitWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

void UReconnectWaitWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
    WidgetTree->RootWidget = Root;

    // 어두운 바탕 + 하늘색 테두리 띠
    FSlateRoundedBoxBrush Brush(Hex(20, 26, 48, 230), Hex(120, 190, 255), 3.f);
    Brush.OutlineSettings.CornerRadii = FVector4(20.f, 20.f, 20.f, 20.f);
    Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;

    Banner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Banner"));
    Banner->SetBrush(Brush);
    Banner->SetPadding(FMargin(30.f, 10.f, 30.f, 12.f));
    if (UOverlaySlot* S = Root->AddChildToOverlay(Banner))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Top);
        S->SetPadding(FMargin(0.f, TopOffset, 0.f, 0.f));
    }

    UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Lines"));
    Banner->SetContent(Lines);

    auto MakeFont = [this](int32 Size, int32 Outline)
    {
        FSlateFontInfo Font = BoldFont ? FSlateFontInfo(BoldFont.Get(), Size) : FCoreStyle::GetDefaultFontStyle("Bold", Size);
        Font.OutlineSettings.OutlineSize = Outline;
        Font.OutlineSettings.OutlineColor = Hex(10, 14, 30);
        return Font;
    };

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
    TitleText->SetFont(MakeFont(24, 2));
    TitleText->SetColorAndOpacity(FSlateColor(Hex(170, 215, 255)));
    TitleText->SetJustification(ETextJustify::Center);
    if (UVerticalBoxSlot* S = Lines->AddChildToVerticalBox(TitleText))
    {
        S->SetHorizontalAlignment(HAlign_Center);
    }

    DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Detail"));
    DetailText->SetFont(MakeFont(18, 1));
    DetailText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    DetailText->SetJustification(ETextJustify::Center);
    if (UVerticalBoxSlot* S = Lines->AddChildToVerticalBox(DetailText))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
    }

    // 클릭을 막지 않는다. 대기 중이 아니면 숨긴다(투명도 대신 숨김: 둥근 테두리는 투명도를 따르지 않는다).
    SetVisibility(ESlateVisibility::HitTestInvisible);
    Banner->SetVisibility(ESlateVisibility::Collapsed);
}

void UReconnectWaitWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    const EFinalRole Missing = GS ? GS->GetReconnectWaitRole() : EFinalRole::None;
    const bool bWaiting = Missing != EFinalRole::None && GS->GetGamePhase() != EGamePhase::Ended;

    if (!bWaiting)
    {
        if (Banner && Banner->GetVisibility() != ESlateVisibility::Collapsed)
        {
            Banner->SetVisibility(ESlateVisibility::Collapsed);
        }
        LastRole = -1;
        LastShownSeconds = -1;
        return;
    }

    const int32 Seconds = FMath::CeilToInt(GS->GetReconnectWaitRemainingSeconds());
    if (static_cast<int32>(Missing) == LastRole && Seconds == LastShownSeconds)
    {
        return;
    }
    LastRole = static_cast<int32>(Missing);
    LastShownSeconds = Seconds;

    const bool bSeekersGone = Missing == EFinalRole::Seeker;
    TitleText->SetText(FText::FromString(bSeekersGone ? TEXT("술래가 모두 나갔어요") : TEXT("숨는 사람이 모두 나갔어요")));
    DetailText->SetText(FText::FromString(FString::Printf(TEXT("%d:%02d 안에 돌아오지 않으면 %s 승리"),
        Seconds / 60, Seconds % 60, bSeekersGone ? TEXT("숨는 팀") : TEXT("술래 팀"))));

    if (Banner->GetVisibility() != ESlateVisibility::HitTestInvisible)
    {
        Banner->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}
