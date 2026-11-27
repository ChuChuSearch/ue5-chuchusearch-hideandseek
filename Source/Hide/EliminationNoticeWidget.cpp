#include "EliminationNoticeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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

    constexpr int32 MaxEntries = 4;
    constexpr float FadeInSeconds = 0.2f;
    constexpr float HoldSeconds = 3.0f;
    constexpr float FadeOutSeconds = 0.6f;

    // 화면 위 시간 표시 아래쯤
    constexpr float TopOffset = 190.f;
}

TSharedRef<SWidget> UEliminationNoticeWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

FSlateFontInfo UEliminationNoticeWidget::MakeFont(int32 Size, const FLinearColor& OutlineColor, int32 OutlineSize) const
{
    FFontOutlineSettings Outline;
    Outline.OutlineSize = OutlineSize;
    Outline.OutlineColor = OutlineColor;

    if (BoldFont)
    {
        return FSlateFontInfo(BoldFont.Get(), Size, NAME_None, Outline);
    }

    FSlateFontInfo Fallback = FCoreStyle::GetDefaultFontStyle("Bold", Size);
    Fallback.OutlineSettings = Outline;
    return Fallback;
}

void UEliminationNoticeWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
    WidgetTree->RootWidget = Root;

    List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("List"));
    if (UOverlaySlot* S = Root->AddChildToOverlay(List))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Top);
        S->SetPadding(FMargin(0.f, TopOffset, 0.f, 0.f));
    }

    // 클릭을 막지 않도록
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UEliminationNoticeWidget::AddNotice(const FString& RunnerName, int32 RemainingRunners)
{
    AddMessage(RunnerName.IsEmpty() ? TEXT("러너") : RunnerName, TEXT(" 님이 잡혔어요!"),
        RemainingRunners > 0 ? FString::Printf(TEXT("남은 러너 %d명"), RemainingRunners) : FString(TEXT("러너 전멸!")),
        Hex(255, 128, 128), Hex(255, 160, 160));
}

void UEliminationNoticeWidget::AddMessage(const FString& Highlight, const FString& Message, const FString& SubText,
    const FLinearColor& BorderColor, const FLinearColor& SubColor)
{
    if (!List)
    {
        return;
    }

    // 너무 많이 쌓이면 가장 오래된 것부터 지운다
    while (Entries.Num() >= MaxEntries)
    {
        Entries[0]->RemoveFromParent();
        Entries.RemoveAt(0);
        EntryAges.RemoveAt(0);
    }

    // 어두운 알약 모양 바탕 + 색 테두리(탈락은 코랄, 연결 안내는 하늘색 등)
    FSlateRoundedBoxBrush Brush(Hex(40, 18, 30, 225), BorderColor, 3.f);
    Brush.OutlineSettings.CornerRadii = FVector4(24.f, 24.f, 24.f, 24.f);
    Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;

    UBorder* Pill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Pill->SetBrush(Brush);
    Pill->SetPadding(FMargin(28.f, 10.f, 28.f, 12.f));

    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Pill->SetContent(Line);

    const FLinearColor Outline = Hex(60, 20, 30);

    UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    NameText->SetText(FText::FromString(Highlight));
    NameText->SetFont(MakeFont(24, Outline, 2));
    NameText->SetColorAndOpacity(FSlateColor(Hex(255, 214, 90)));
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(NameText))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    UTextBlock* MsgText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    MsgText->SetText(FText::FromString(Message));
    MsgText->SetFont(MakeFont(24, Outline, 2));
    MsgText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(MsgText))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (!SubText.IsEmpty())
    {
        UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        CountText->SetText(FText::FromString(SubText));
        CountText->SetFont(MakeFont(18, Outline, 1));
        CountText->SetColorAndOpacity(FSlateColor(SubColor));
        if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(CountText))
        {
            S->SetPadding(FMargin(18.f, 0.f, 0.f, 0.f));
            S->SetVerticalAlignment(VAlign_Center);
        }
    }

    if (UVerticalBoxSlot* S = List->AddChildToVerticalBox(Pill))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetPadding(FMargin(0.f, 4.f));
    }

    Pill->SetRenderOpacity(0.f);
    Entries.Add(Pill);
    EntryAges.Add(0.f);
}

void UEliminationNoticeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    for (int32 i = Entries.Num() - 1; i >= 0; --i)
    {
        EntryAges[i] += InDeltaTime;
        const float Age = EntryAges[i];

        float Alpha = 1.f;
        if (Age < FadeInSeconds)
        {
            Alpha = Age / FadeInSeconds;
        }
        else if (Age > FadeInSeconds + HoldSeconds)
        {
            Alpha = 1.f - (Age - FadeInSeconds - HoldSeconds) / FadeOutSeconds;
        }

        if (Alpha <= 0.f)
        {
            Entries[i]->RemoveFromParent();
            Entries.RemoveAt(i);
            EntryAges.RemoveAt(i);
            continue;
        }

        Entries[i]->SetRenderOpacity(FMath::Clamp(Alpha, 0.f, 1.f));
    }
}
