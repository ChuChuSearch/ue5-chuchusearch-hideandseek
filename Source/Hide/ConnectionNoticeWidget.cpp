#include "ConnectionNoticeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"

namespace
{
    const TCHAR* BoldFontPath = TEXT("/Game/Game/Fonts/Maplestory_Bold_Font.Maplestory_Bold_Font");
    const TCHAR* LightFontPath = TEXT("/Game/Game/Fonts/Maplestory_Light_Font.Maplestory_Light_Font");
    const TCHAR* ClickSoundPath = TEXT("/Game/Game/Sounds/UI/SFX_UI_Click.SFX_UI_Click");

    FLinearColor Hex(uint8 R, uint8 G, uint8 B, uint8 A = 255)
    {
        return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
    }

    const FLinearColor PanelOuterFill = Hex(255, 236, 196);
    const FLinearColor PanelOuterLine = Hex(236, 152, 42);
    const FLinearColor PanelInnerFill = Hex(255, 252, 242);
    const FLinearColor PanelInnerLine = Hex(240, 196, 128);
    const FLinearColor RibbonFill = Hex(146, 92, 232);
    const FLinearColor RibbonLine = Hex(92, 46, 170);
    const FLinearColor LabelColor = Hex(122, 74, 30);
    const FLinearColor AccentColor = Hex(255, 128, 128);
    const FLinearColor AccentHover = Hex(255, 158, 150);
    const FLinearColor AccentDark = Hex(214, 82, 90);

    FSlateBrush RoundBox(const FLinearColor& Fill, float Radius, const FLinearColor& Line = FLinearColor::Transparent, float LineWidth = 0.f)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }
}

void UConnectionNoticeWidget::SetMessage(const FString& InMessage)
{
    Message = InMessage;
    if (MessageText)
    {
        MessageText->SetText(FText::FromString(Message));
    }
}

TSharedRef<SWidget> UConnectionNoticeWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

void UConnectionNoticeWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    LightFont = LoadObject<UObject>(nullptr, LightFontPath);
    ClickSound = LoadObject<USoundBase>(nullptr, ClickSoundPath);

    auto MakeFont = [this](bool bBold, int32 Size, const FLinearColor& Outline = FLinearColor::Transparent, int32 OutlineSize = 0)
    {
        const UObject* FontObj = bBold ? BoldFont.Get() : LightFont.Get();
        FSlateFontInfo Font = FontObj ? FSlateFontInfo(FontObj, Size) : FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
        Font.OutlineSettings.OutlineSize = OutlineSize;
        Font.OutlineSettings.OutlineColor = Outline;
        return Font;
    };

    UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
    Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
    Dim->SetHorizontalAlignment(HAlign_Center);
    Dim->SetVerticalAlignment(VAlign_Center);
    WidgetTree->RootWidget = Dim;

    UOverlay* Card = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Card"));
    Dim->SetContent(Card);

    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Outer"));
    Outer->SetBrush(RoundBox(PanelOuterFill, 28.f, PanelOuterLine, 5.f));
    Outer->SetPadding(FMargin(10.f));
    if (UOverlaySlot* S = Card->AddChildToOverlay(Outer))
    {
        S->SetPadding(FMargin(0.f, 26.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
    }

    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Inner"));
    Inner->SetBrush(RoundBox(PanelInnerFill, 20.f, PanelInnerLine, 2.f));
    Inner->SetPadding(FMargin(44.f, 46.f, 44.f, 24.f));
    Outer->SetContent(Inner);

    USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Width"));
    Width->SetMinDesiredWidth(440.f);
    Inner->SetContent(Width);

    UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Body"));
    Width->SetContent(Body);

    UBorder* Ribbon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Ribbon"));
    Ribbon->SetBrush(RoundBox(RibbonFill, 16.f, RibbonLine, 3.f));
    Ribbon->SetPadding(FMargin(44.f, 7.f, 44.f, 9.f));
    if (UOverlaySlot* S = Card->AddChildToOverlay(Ribbon))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Top);
    }
    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
    Title->SetText(FText::FromString(TEXT("알림")));
    Title->SetFont(MakeFont(true, 28, RibbonLine, 3));
    Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Ribbon->SetContent(Title);

    MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Message"));
    MessageText->SetText(FText::FromString(Message));
    MessageText->SetFont(MakeFont(true, 20));
    MessageText->SetColorAndOpacity(FSlateColor(LabelColor));
    MessageText->SetJustification(ETextJustify::Center);
    if (UVerticalBoxSlot* S = Body->AddChildToVerticalBox(MessageText))
    {
        S->SetHorizontalAlignment(HAlign_Center);
    }

    UButton* Confirm = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Confirm"));
    {
        FButtonStyle Style = Confirm->GetStyle();
        Style.SetNormal(RoundBox(AccentColor, 20.f, AccentDark, 3.f));
        Style.SetHovered(RoundBox(AccentHover, 20.f, AccentDark, 3.f));
        Style.SetPressed(RoundBox(AccentDark, 20.f, AccentDark, 3.f));
        Style.SetNormalPadding(FMargin(48.f, 8.f));
        Style.SetPressedPadding(FMargin(48.f, 9.f, 48.f, 7.f));
        if (ClickSound)
        {
            FSlateSound Click;
            Click.SetResourceObject(ClickSound);
            Style.SetPressedSound(Click);
        }
        Confirm->SetStyle(Style);
    }
    UTextBlock* ConfirmText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmText"));
    ConfirmText->SetText(FText::FromString(TEXT("확인")));
    ConfirmText->SetFont(MakeFont(true, 20, AccentDark, 1));
    ConfirmText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Confirm->SetContent(ConfirmText);
    Confirm->OnClicked.AddDynamic(this, &UConnectionNoticeWidget::OnConfirmClicked);
    if (UVerticalBoxSlot* S = Body->AddChildToVerticalBox(Confirm))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));
    }
}

FReply UConnectionNoticeWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 팝업이 떠 있는 동안 뒤의 접속 화면이 눌리지 않게
    return FReply::Handled();
}

void UConnectionNoticeWidget::OnConfirmClicked()
{
    RemoveFromParent();
}
