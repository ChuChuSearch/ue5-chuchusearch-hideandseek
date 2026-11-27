#include "AnyRoleButtonWidget.h"

#include "MyPlayerController.h"
#include "MyPlayerState.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"

namespace
{
    const TCHAR* BoldFontPath = TEXT("/Game/Game/Fonts/Maplestory_Bold_Font.Maplestory_Bold_Font");
    const TCHAR* ClickSoundPath = TEXT("/Game/Game/Sounds/UI/SFX_UI_Click.SFX_UI_Click");
    const TCHAR* HoverSoundPath = TEXT("/Game/Game/Sounds/UI/SFX_UI_Hover.SFX_UI_Hover");

    FLinearColor Hex(uint8 R, uint8 G, uint8 B, uint8 A = 255)
    {
        return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
    }

    const FLinearColor IdleFill = Hex(255, 248, 232);
    const FLinearColor IdleHover = Hex(255, 236, 210);
    const FLinearColor IdleLine = Hex(236, 152, 42);
    const FLinearColor LabelColor = Hex(122, 74, 30);
    const FLinearColor AccentColor = Hex(255, 128, 128);
    const FLinearColor AccentHover = Hex(255, 158, 150);
    const FLinearColor AccentDark = Hex(214, 82, 90);

    FSlateBrush RoundBox(const FLinearColor& Fill, float Radius, const FLinearColor& Line, float LineWidth)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }
}

TSharedRef<SWidget> UAnyRoleButtonWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

void UAnyRoleButtonWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    ClickSound = LoadObject<USoundBase>(nullptr, ClickSoundPath);
    HoverSound = LoadObject<USoundBase>(nullptr, HoverSoundPath);

    Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AnyButton"));
    WidgetTree->RootWidget = Button;
    Button->OnClicked.AddDynamic(this, &UAnyRoleButtonWidget::OnClicked);

    Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AnyLabel"));
    Label->SetText(FText::FromString(TEXT("상관없음")));
    Label->SetJustification(ETextJustify::Center);
    Button->SetContent(Label);

    ApplyStyle(false);
}

void UAnyRoleButtonWidget::ApplyStyle(bool bSelected)
{
    if (!Button || !Label)
    {
        return;
    }

    FButtonStyle Style = Button->GetStyle();
    if (bSelected)
    {
        Style.SetNormal(RoundBox(AccentColor, 24.f, AccentDark, 3.f));
        Style.SetHovered(RoundBox(AccentHover, 24.f, AccentDark, 3.f));
        Style.SetPressed(RoundBox(AccentDark, 24.f, AccentDark, 3.f));
    }
    else
    {
        Style.SetNormal(RoundBox(IdleFill, 24.f, IdleLine, 3.f));
        Style.SetHovered(RoundBox(IdleHover, 24.f, IdleLine, 3.f));
        Style.SetPressed(RoundBox(IdleHover, 24.f, AccentDark, 3.f));
    }
    Style.SetNormalPadding(FMargin(16.f, 4.f));
    Style.SetPressedPadding(FMargin(16.f, 5.f, 16.f, 3.f));
    if (ClickSound)
    {
        FSlateSound Click;
        Click.SetResourceObject(ClickSound);
        Style.SetPressedSound(Click);
    }
    if (HoverSound)
    {
        FSlateSound Hover;
        Hover.SetResourceObject(HoverSound);
        Style.SetHoveredSound(Hover);
    }
    Button->SetStyle(Style);

    FSlateFontInfo Font = BoldFont ? FSlateFontInfo(BoldFont.Get(), 22) : FCoreStyle::GetDefaultFontStyle("Bold", 22);
    if (bSelected)
    {
        Font.OutlineSettings.OutlineSize = 2;
        Font.OutlineSettings.OutlineColor = AccentDark;
    }
    Label->SetFont(Font);
    Label->SetColorAndOpacity(FSlateColor(bSelected ? FLinearColor::White : LabelColor));
}

void UAnyRoleButtonWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 내 지망이 '상관없음'인지에 따라 모양을 바꾼다(바뀔 때만)
    const APlayerController* PC = GetOwningPlayer();
    const AMyPlayerState* PS = PC ? PC->GetPlayerState<AMyPlayerState>() : nullptr;
    const int32 Selected = (PS && PS->GetPreference() == ERolePreference::Any) ? 1 : 0;
    if (Selected != LastSelected)
    {
        LastSelected = Selected;
        ApplyStyle(Selected == 1);
    }
}

void UAnyRoleButtonWidget::OnClicked()
{
    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
    {
        PC->RequestSetPreference(ERolePreference::Any);
    }
}
