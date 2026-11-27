#include "SpectateHudWidget.h"

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
}

TSharedRef<SWidget> USpectateHudWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

void USpectateHudWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);

    auto MakeFont = [this](int32 Size, int32 Outline)
    {
        FSlateFontInfo Font = BoldFont ? FSlateFontInfo(BoldFont.Get(), Size) : FCoreStyle::GetDefaultFontStyle("Bold", Size);
        Font.OutlineSettings.OutlineSize = Outline;
        Font.OutlineSettings.OutlineColor = Hex(20, 12, 40);
        return Font;
    };

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
    WidgetTree->RootWidget = Root;

    // 어두운 보라 바탕 + 보라 테두리(유령 색)
    FSlateRoundedBoxBrush Brush(Hex(30, 18, 50, 225), Hex(146, 92, 232), 3.f);
    Brush.OutlineSettings.CornerRadii = FVector4(22.f, 22.f, 22.f, 22.f);
    Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;

    UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
    Card->SetBrush(Brush);
    Card->SetPadding(FMargin(30.f, 10.f, 30.f, 12.f));
    if (UOverlaySlot* S = Root->AddChildToOverlay(Card))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Bottom);
        S->SetPadding(FMargin(0.f, 0.f, 0.f, 40.f));
    }

    UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Lines"));
    Card->SetContent(Lines);

    // 위 줄: 관전 중  [이름]  [역할]  (1/3)
    UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Top"));
    if (UVerticalBoxSlot* S = Lines->AddChildToVerticalBox(Top))
    {
        S->SetHorizontalAlignment(HAlign_Center);
    }

    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Label"));
    Label->SetText(FText::FromString(TEXT("관전 중")));
    Label->SetFont(MakeFont(18, 1));
    Label->SetColorAndOpacity(FSlateColor(Hex(200, 180, 255)));
    if (UHorizontalBoxSlot* S = Top->AddChildToHorizontalBox(Label))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
    }

    NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Name"));
    NameText->SetFont(MakeFont(26, 2));
    NameText->SetColorAndOpacity(FSlateColor(Hex(255, 214, 90)));
    if (UHorizontalBoxSlot* S = Top->AddChildToHorizontalBox(NameText))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    RoleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Role"));
    RoleText->SetFont(MakeFont(18, 1));
    if (UHorizontalBoxSlot* S = Top->AddChildToHorizontalBox(RoleText))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
    }

    CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Count"));
    CountText->SetFont(MakeFont(16, 1));
    CountText->SetColorAndOpacity(FSlateColor(Hex(180, 170, 210)));
    if (UHorizontalBoxSlot* S = Top->AddChildToHorizontalBox(CountText))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
    }

    // 아래 줄: 조작 안내
    UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Hint"));
    Hint->SetText(FText::FromString(TEXT("마우스로 둘러보기 · 좌클릭/→ 다음 · 우클릭/← 이전 · V 관전 끄기")));
    Hint->SetFont(MakeFont(14, 1));
    Hint->SetColorAndOpacity(FSlateColor(Hex(220, 215, 235)));
    if (UVerticalBoxSlot* S = Lines->AddChildToVerticalBox(Hint))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
    }

    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USpectateHudWidget::SetTarget(const FString& Name, const FString& InRoleText, const FLinearColor& RoleColor, int32 Index, int32 Count)
{
    if (!NameText)
    {
        return;
    }

    NameText->SetText(FText::FromString(Name));
    RoleText->SetText(FText::FromString(InRoleText));
    RoleText->SetColorAndOpacity(FSlateColor(RoleColor));
    CountText->SetText(FText::FromString(Count > 1 ? FString::Printf(TEXT("%d / %d"), Index + 1, Count) : FString()));
}
