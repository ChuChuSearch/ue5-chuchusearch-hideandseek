#include "LobbySettingsWidget.h"

#include "MyPlayerController.h"
#include "MyPlayerState.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameStateBase.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"

namespace
{
    const TCHAR* BoldFontPath = TEXT("/Game/Game/Fonts/Maplestory_Bold_Font.Maplestory_Bold_Font");
    const TCHAR* LightFontPath = TEXT("/Game/Game/Fonts/Maplestory_Light_Font.Maplestory_Light_Font");
    const TCHAR* ClickSoundPath = TEXT("/Game/Game/Sounds/UI/SFX_UI_Click.SFX_UI_Click");
    const TCHAR* HoverSoundPath = TEXT("/Game/Game/Sounds/UI/SFX_UI_Hover.SFX_UI_Hover");

    FLinearColor Hex(uint8 R, uint8 G, uint8 B, uint8 A = 255)
    {
        return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
    }

    // 설정 창과 같은 색
    const FLinearColor PanelOuterFill = Hex(255, 236, 196);
    const FLinearColor PanelOuterLine = Hex(236, 152, 42);
    const FLinearColor PanelInnerFill = Hex(255, 252, 242);
    const FLinearColor PanelInnerLine = Hex(240, 196, 128);
    const FLinearColor RibbonFill = Hex(146, 92, 232);
    const FLinearColor RibbonLine = Hex(92, 46, 170);
    const FLinearColor LabelColor = Hex(122, 74, 30);
    const FLinearColor SubLabelColor = Hex(122, 74, 30, 160);
    const FLinearColor AccentColor = Hex(255, 128, 128);
    const FLinearColor AccentHover = Hex(255, 158, 150);
    const FLinearColor AccentDark = Hex(214, 82, 90);
    const FLinearColor TrackFill = Hex(244, 228, 198);
    const FLinearColor TrackLine = Hex(218, 180, 120);
    const FLinearColor TanFill = Hex(240, 220, 190);
    const FLinearColor TanHover = Hex(248, 232, 206);
    const FLinearColor TanLine = Hex(200, 160, 110);

    constexpr float LabelWidth = 150.f;
    constexpr float ValueWidth = 150.f;
    constexpr float ArrowSize = 44.f;

    FSlateBrush RoundBox(const FLinearColor& Fill, float Radius, const FLinearColor& Line = FLinearColor::Transparent, float LineWidth = 0.f)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }
}

void ULobbySettingStepHandler::HandleClicked()
{
    if (ULobbySettingsWidget* Widget = Owner.Get())
    {
        Widget->StepField(Field, Step);
    }
}

TSharedRef<SWidget> ULobbySettingsWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

FSlateFontInfo ULobbySettingsWidget::MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor, int32 OutlineSize) const
{
    const UObject* FontObj = bBold ? BoldFont.Get() : LightFont.Get();
    FSlateFontInfo Font = FontObj ? FSlateFontInfo(FontObj, Size) : FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
    if (OutlineSize > 0)
    {
        Font.OutlineSettings.OutlineSize = OutlineSize;
        Font.OutlineSettings.OutlineColor = OutlineColor;
    }
    return Font;
}

UButton* ULobbySettingsWidget::MakeTextButton(const FString& Label, const FLinearColor& Fill, const FLinearColor& Line, const FLinearColor& TextColor)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    FButtonStyle Style = Button->GetStyle();
    Style.SetNormal(RoundBox(Fill, 20.f, Line, 3.f));
    Style.SetHovered(RoundBox(FMath::Lerp(Fill, FLinearColor::White, 0.18f), 20.f, Line, 3.f));
    Style.SetPressed(RoundBox(Line, 20.f, Line, 3.f));
    Style.SetDisabled(RoundBox(Hex(230, 222, 210), 20.f, Hex(210, 200, 186), 3.f));
    Style.SetNormalPadding(FMargin(10.f, 6.f));
    Style.SetPressedPadding(FMargin(10.f, 7.f, 10.f, 5.f));
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

    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(FText::FromString(Label));
    Text->SetFont(MakeFont(true, 18));
    Text->SetColorAndOpacity(FSlateColor(TextColor));
    Text->SetJustification(ETextJustify::Center);
    Button->SetContent(Text);
    return Button;
}

UButton* ULobbySettingsWidget::MakeArrowButton(const FString& Label, ELobbySettingField Field, int32 Step)
{
    UButton* Button = MakeTextButton(Label, AccentColor, AccentDark, FLinearColor::White);

    ULobbySettingStepHandler* Handler = NewObject<ULobbySettingStepHandler>(this);
    Handler->Owner = this;
    Handler->Field = Field;
    Handler->Step = Step;
    StepHandlers.Add(Handler);
    Button->OnClicked.AddDynamic(Handler, &ULobbySettingStepHandler::HandleClicked);
    return Button;
}

void ULobbySettingsWidget::AddRow(UVerticalBox* Box, ELobbySettingField Field)
{
    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LabelBox->SetWidthOverride(LabelWidth);
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(FLobbySettingRules::GetLabel(Field)));
    Label->SetFont(MakeFont(true, 20));
    Label->SetColorAndOpacity(FSlateColor(LabelColor));
    LabelBox->SetContent(Label);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(LabelBox))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    FRowWidgets Row;

    auto AddArrow = [&](const FString& ArrowLabel, int32 Step) -> UButton*
    {
        USizeBox* ArrowBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        ArrowBox->SetWidthOverride(ArrowSize);
        ArrowBox->SetHeightOverride(ArrowSize);
        UButton* Arrow = MakeArrowButton(ArrowLabel, Field, Step);
        ArrowBox->SetContent(Arrow);
        if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(ArrowBox))
        {
            S->SetVerticalAlignment(VAlign_Center);
        }
        return Arrow;
    };

    Row.Prev = AddArrow(TEXT("<"), -1);

    // 값 칸(크림색 둥근 상자)
    USizeBox* ValueBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    ValueBox->SetWidthOverride(ValueWidth);
    ValueBox->SetHeightOverride(ArrowSize);
    UBorder* ValueBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    ValueBg->SetBrush(RoundBox(TrackFill, 14.f, TrackLine, 2.f));
    ValueBg->SetHorizontalAlignment(HAlign_Center);
    ValueBg->SetVerticalAlignment(VAlign_Center);
    Row.Value = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Row.Value->SetFont(MakeFont(true, 20));
    Row.Value->SetColorAndOpacity(FSlateColor(LabelColor));
    ValueBg->SetContent(Row.Value);
    ValueBox->SetContent(ValueBg);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(ValueBox))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(8.f, 0.f));
    }

    Row.Next = AddArrow(TEXT(">"), +1);

    Rows.Add(Row);

    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Line))
    {
        S->SetPadding(FMargin(0.f, 6.f));
    }
}

void ULobbySettingsWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    LightFont = LoadObject<UObject>(nullptr, LightFontPath);
    ClickSound = LoadObject<USoundBase>(nullptr, ClickSoundPath);
    HoverSound = LoadObject<USoundBase>(nullptr, HoverSoundPath);

    // 뒤를 어둡게 덮어 대기실 버튼이 눌리지 않게 한다
    UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
    Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.5f));
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
    Inner->SetPadding(FMargin(40.f, 44.f, 40.f, 22.f));
    Outer->SetContent(Inner);

    UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Body"));
    Inner->SetContent(Body);

    UBorder* Ribbon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Ribbon"));
    Ribbon->SetBrush(RoundBox(RibbonFill, 16.f, RibbonLine, 3.f));
    Ribbon->SetPadding(FMargin(44.f, 7.f, 44.f, 9.f));
    if (UOverlaySlot* S = Card->AddChildToOverlay(Ribbon))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Top);
    }
    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
    Title->SetText(FText::FromString(TEXT("게임 설정")));
    Title->SetFont(MakeFont(true, 28, RibbonLine, 3));
    Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Ribbon->SetContent(Title);

    for (int32 i = 0; i < static_cast<int32>(ELobbySettingField::Count); ++i)
    {
        AddRow(Body, static_cast<ELobbySettingField>(i));
    }

    // 전체 게임 시간
    TotalText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TotalText"));
    TotalText->SetFont(MakeFont(true, 18));
    TotalText->SetColorAndOpacity(FSlateColor(AccentDark));
    if (UVerticalBoxSlot* S = Body->AddChildToVerticalBox(TotalText))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
    }

    NoteText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NoteText"));
    NoteText->SetFont(MakeFont(false, 14));
    NoteText->SetColorAndOpacity(FSlateColor(SubLabelColor));
    if (UVerticalBoxSlot* S = Body->AddChildToVerticalBox(NoteText))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
    }

    // 아래 버튼: [기본값으로](방장) [닫기]
    UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
    if (UVerticalBoxSlot* S = Body->AddChildToVerticalBox(Buttons))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
    }

    USizeBox* ResetBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    ResetBox->SetWidthOverride(150.f);
    ResetBox->SetHeightOverride(46.f);
    ResetButton = MakeTextButton(TEXT("기본값으로"), TanFill, TanLine, LabelColor);
    ResetButton->OnClicked.AddDynamic(this, &ULobbySettingsWidget::OnResetClicked);
    ResetBox->SetContent(ResetButton);
    if (UHorizontalBoxSlot* S = Buttons->AddChildToHorizontalBox(ResetBox))
    {
        S->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
    }

    USizeBox* CloseBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    CloseBox->SetWidthOverride(150.f);
    CloseBox->SetHeightOverride(46.f);
    UButton* CloseButton = MakeTextButton(TEXT("닫기"), AccentColor, AccentDark, FLinearColor::White);
    CloseButton->OnClicked.AddDynamic(this, &ULobbySettingsWidget::OnCloseClicked);
    CloseBox->SetContent(CloseButton);
    Buttons->AddChildToHorizontalBox(CloseBox);

    Refresh();
}

void ULobbySettingsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    Refresh();
}

FReply ULobbySettingsWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 어두운 바깥을 눌러도 뒤의 대기실 버튼이 눌리지 않게 막는다
    return FReply::Handled();
}

void ULobbySettingsWidget::Refresh()
{
    const AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer());
    if (!PC)
    {
        return;
    }

    const AMyPlayerState* PS = PC->GetPlayerState<AMyPlayerState>();
    const int32 Host = (PS && PS->IsHost()) ? 1 : 0;
    const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    const int32 PlayerCount = GS ? GS->PlayerArray.Num() : 0;
    const FLobbyGameSettings& Settings = PC->GetLobbyGameSettings();

    if (bShownAny && Settings == ShownSettings && Host == ShownHost && PlayerCount == ShownPlayerCount)
    {
        return;
    }
    bShownAny = true;
    ShownSettings = Settings;
    ShownHost = Host;
    ShownPlayerCount = PlayerCount;

    const bool bHost = Host == 1;
    for (int32 i = 0; i < Rows.Num(); ++i)
    {
        const ELobbySettingField Field = static_cast<ELobbySettingField>(i);
        const int32 Value = FLobbySettingRules::GetValue(Settings, Field);
        Rows[i].Value->SetText(FText::FromString(PC->HasLobbyGameSettings() ? FLobbySettingRules::FormatValue(Field, Value) : TEXT("…")));

        // 방장만 < >가 보인다(자리는 남겨 줄이 흔들리지 않게)
        const ESlateVisibility ArrowVis = bHost ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
        Rows[i].Prev->SetVisibility(ArrowVis);
        Rows[i].Next->SetVisibility(ArrowVis);

        // 끝이면 못 누르게. 최대 인원은 지금 인원보다 줄일 수 없다.
        const int32 PrevValue = FLobbySettingRules::StepValue(Field, Value, -1);
        const int32 NextValue = FLobbySettingRules::StepValue(Field, Value, +1);
        bool bCanPrev = PrevValue != Value;
        if (Field == ELobbySettingField::MaxPlayers && PrevValue < PlayerCount)
        {
            bCanPrev = false;
        }
        Rows[i].Prev->SetIsEnabled(bCanPrev);
        Rows[i].Next->SetIsEnabled(NextValue != Value);
    }

    TotalText->SetText(FText::FromString(FString::Printf(TEXT("전체 게임 시간  %s"),
        *FLobbySettingRules::FormatTime(Settings.GetTotalSeconds()))));
    NoteText->SetText(FText::FromString(bHost
        ? TEXT("바꾸면 모두의 준비가 풀려요")
        : TEXT("게임 설정은 방장만 바꿀 수 있어요")));
    ResetButton->SetVisibility(bHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void ULobbySettingsWidget::StepField(ELobbySettingField Field, int32 Step)
{
    AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer());
    if (!PC || !PC->HasLobbyGameSettings())
    {
        return;
    }

    FLobbyGameSettings NewSettings = PC->GetLobbyGameSettings();
    const int32 Current = FLobbySettingRules::GetValue(NewSettings, Field);
    const int32 Next = FLobbySettingRules::StepValue(Field, Current, Step);
    if (Next == Current)
    {
        return;
    }

    FLobbySettingRules::SetValue(NewSettings, Field, Next);
    PC->ServerSetLobbyGameSettings(NewSettings, false);
}

void ULobbySettingsWidget::OnResetClicked()
{
    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
    {
        PC->ServerSetLobbyGameSettings(PC->GetLobbyGameSettings(), true);
    }
}

void ULobbySettingsWidget::OnCloseClicked()
{
    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
    {
        PC->ToggleLobbySettingsWindow();
    }
}
