#include "RoleChoiceButtonWidget.h"

#include "MyPlayerController.h"

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

    constexpr float Radius = 26.f;

    FSlateBrush RoundBox(const FLinearColor& Fill, const FLinearColor& Line, float LineWidth)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }

    FLinearColor Lighten(const FLinearColor& C, float Amount)
    {
        return FMath::Lerp(C, FLinearColor::White, Amount).CopyWithNewOpacity(C.A);
    }
}

void URoleChoiceButtonWidget::Setup(ERolePreference InPreference, const FString& InLabel, const FLinearColor& InSelectedFill, const FLinearColor& InSelectedLine, int32 InFontSize)
{
    Preference = InPreference;
    LabelText = InLabel;
    SelectedFill = InSelectedFill;
    SelectedLine = InSelectedLine;
    FontSize = InFontSize;

    if (Label)
    {
        Label->SetText(FText::FromString(LabelText));
    }
    LastSelected = -1;
}

TSharedRef<SWidget> URoleChoiceButtonWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

void URoleChoiceButtonWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    ClickSound = LoadObject<USoundBase>(nullptr, ClickSoundPath);
    HoverSound = LoadObject<USoundBase>(nullptr, HoverSoundPath);

    Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RoleButton"));
    WidgetTree->RootWidget = Button;
    Button->OnClicked.AddDynamic(this, &URoleChoiceButtonWidget::OnClicked);

    Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RoleLabel"));
    Label->SetText(FText::FromString(LabelText));
    Label->SetJustification(ETextJustify::Center);
    Button->SetContent(Label);

    ApplyStyle(false);
}

void URoleChoiceButtonWidget::ApplyStyle(bool bSelected)
{
    if (!Button || !Label)
    {
        return;
    }

    FButtonStyle Style = Button->GetStyle();
    if (bSelected)
    {
        Style.SetNormal(RoundBox(SelectedFill, SelectedLine, 3.f));
        Style.SetHovered(RoundBox(Lighten(SelectedFill, 0.15f), SelectedLine, 3.f));
        Style.SetPressed(RoundBox(SelectedLine, SelectedLine, 3.f));
    }
    else
    {
        Style.SetNormal(RoundBox(IdleFill, IdleLine, 3.f));
        Style.SetHovered(RoundBox(IdleHover, IdleLine, 3.f));
        Style.SetPressed(RoundBox(IdleHover, SelectedLine, 3.f));
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

    FSlateFontInfo Font = BoldFont ? FSlateFontInfo(BoldFont.Get(), FontSize) : FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
    if (bSelected)
    {
        Font.OutlineSettings.OutlineSize = 2;
        Font.OutlineSettings.OutlineColor = SelectedLine;
    }
    Label->SetFont(Font);
    Label->SetColorAndOpacity(FSlateColor(bSelected ? FLinearColor::White : LabelColor));
}

void URoleChoiceButtonWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 내 지망이 이 버튼인지에 따라 모양을 바꾼다(바뀔 때만)
    const APlayerController* PC = GetOwningPlayer();
    const AMyPlayerState* PS = PC ? PC->GetPlayerState<AMyPlayerState>() : nullptr;
    const int32 Selected = (PS && PS->GetPreference() == Preference) ? 1 : 0;
    if (Selected != LastSelected)
    {
        LastSelected = Selected;
        ApplyStyle(Selected == 1);
    }
}

void URoleChoiceButtonWidget::OnClicked()
{
    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
    {
        PC->RequestSetPreference(Preference);
    }
}
