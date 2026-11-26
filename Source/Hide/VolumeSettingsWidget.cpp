#include "VolumeSettingsWidget.h"

#include "MyPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"

namespace
{
    const TCHAR* ClickSoundPath = TEXT("/Game/Game/Sounds/UI/SFX_UI_Click.SFX_UI_Click");
    const TCHAR* HoverSoundPath = TEXT("/Game/Game/Sounds/UI/SFX_UI_Hover.SFX_UI_Hover");
    const TCHAR* BoldFontPath = TEXT("/Game/Game/Fonts/Maplestory_Bold_Font.Maplestory_Bold_Font");
    const TCHAR* LightFontPath = TEXT("/Game/Game/Fonts/Maplestory_Light_Font.Maplestory_Light_Font");

    // 대기실/닉네임 입력 UI에서 따온 색
    FLinearColor Hex(uint8 R, uint8 G, uint8 B, uint8 A = 255)
    {
        return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
    }

    const FLinearColor PanelOuterFill = Hex(255, 236, 196);  // 바깥 테두리 안쪽(연한 살구)
    const FLinearColor PanelOuterLine = Hex(236, 152, 42);   // 주황 테두리
    const FLinearColor PanelInnerFill = Hex(255, 252, 242);  // 크림색 본문
    const FLinearColor PanelInnerLine = Hex(240, 196, 128);  // 안쪽 얇은 금색 선
    const FLinearColor RibbonFill = Hex(146, 92, 232);       // 보라 리본
    const FLinearColor RibbonLine = Hex(92, 46, 170);
    const FLinearColor LabelColor = Hex(122, 74, 30);        // 갈색 글씨
    const FLinearColor AccentColor = Hex(255, 128, 128);     // START 버튼 코랄
    const FLinearColor AccentDark = Hex(214, 82, 90);
    const FLinearColor TrackFill = Hex(244, 228, 198);
    const FLinearColor TrackLine = Hex(218, 180, 120);

    FSlateBrush RoundBox(const FLinearColor& Fill, float Radius, const FLinearColor& Line = FLinearColor::Transparent, float LineWidth = 0.f, const FVector2f& Size = FVector2f::ZeroVector)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth, Size);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }

    constexpr float ThumbSize = 28.f;
}

void UVolumeSettingsWidget::InitFromController(AMyPlayerController* InPC)
{
    OwnerPC = InPC;
    RefreshFromController();
}

TSharedRef<SWidget> UVolumeSettingsWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

FSlateFontInfo UVolumeSettingsWidget::MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor, int32 OutlineSize) const
{
    const UObject* FontObj = bBold ? BoldFont.Get() : LightFont.Get();
    FFontOutlineSettings Outline;
    Outline.OutlineSize = OutlineSize;
    Outline.OutlineColor = OutlineColor;

    if (FontObj)
    {
        return FSlateFontInfo(FontObj, Size, NAME_None, Outline);
    }

    // 폰트 에셋을 못 찾으면 엔진 기본 폰트
    FSlateFontInfo Fallback = FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
    Fallback.OutlineSettings = Outline;
    return Fallback;
}

void UVolumeSettingsWidget::BuildTree()
{
    SetIsFocusable(true);

    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    LightFont = LoadObject<UObject>(nullptr, LightFontPath);

    // 화면 전체를 어둡게
    UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
    Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
    Dim->SetHorizontalAlignment(HAlign_Center);
    Dim->SetVerticalAlignment(VAlign_Center);
    WidgetTree->RootWidget = Dim;

    // 패널 위에 리본 제목을 겹쳐 올린다
    UOverlay* Card = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Card"));
    Dim->SetContent(Card);

    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelOuter"));
    Outer->SetBrush(RoundBox(PanelOuterFill, 30.f, PanelOuterLine, 5.f));
    Outer->SetPadding(FMargin(10.f));
    if (UOverlaySlot* S = Card->AddChildToOverlay(Outer))
    {
        S->SetPadding(FMargin(0.f, 28.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
    }

    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelInner"));
    Inner->SetBrush(RoundBox(PanelInnerFill, 22.f, PanelInnerLine, 2.f));
    Inner->SetPadding(FMargin(44.f, 46.f, 44.f, 26.f));
    Outer->SetContent(Inner);

    UBorder* Ribbon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Ribbon"));
    Ribbon->SetBrush(RoundBox(RibbonFill, 16.f, RibbonLine, 3.f));
    Ribbon->SetPadding(FMargin(48.f, 8.f, 48.f, 10.f));
    if (UOverlaySlot* S = Card->AddChildToOverlay(Ribbon))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Top);
    }

    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
    Title->SetText(FText::FromString(TEXT("소리 설정")));
    Title->SetFont(MakeFont(true, 30, RibbonLine, 3));
    Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Ribbon->SetContent(Title);

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Rows"));
    Inner->SetContent(Box);

    const FText Labels[] = {
        FText::FromString(TEXT("전체")),
        FText::FromString(TEXT("배경음")),
        FText::FromString(TEXT("효과음"))
    };
    for (const FText& Label : Labels)
    {
        const FVolumeRow Row = AddRow(Box, Label);
        Sliders.Add(Row.Slider);
        Fills.Add(Row.Fill);
        Percents.Add(Row.Percent);
    }

    Sliders[0]->OnValueChanged.AddDynamic(this, &UVolumeSettingsWidget::OnMasterChanged);
    Sliders[1]->OnValueChanged.AddDynamic(this, &UVolumeSettingsWidget::OnMusicChanged);
    Sliders[2]->OnValueChanged.AddDynamic(this, &UVolumeSettingsWidget::OnSFXChanged);

    // 닫기 버튼: START 버튼 같은 코랄 알약 모양
    UButton* Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
    FButtonStyle Style = Close->GetStyle();
    Style.SetNormal(RoundBox(AccentColor, 26.f, FLinearColor::White, 3.f));
    Style.SetHovered(RoundBox(Hex(255, 158, 150), 26.f, FLinearColor::White, 3.f));
    Style.SetPressed(RoundBox(AccentDark, 26.f, FLinearColor::White, 3.f));
    Style.SetNormalPadding(FMargin(0.f));
    Style.SetPressedPadding(FMargin(0.f, 2.f, 0.f, 0.f));
    if (USoundBase* Click = LoadObject<USoundBase>(nullptr, ClickSoundPath))
    {
        Style.PressedSlateSound.SetResourceObject(Click);
    }
    if (USoundBase* Hover = LoadObject<USoundBase>(nullptr, HoverSoundPath))
    {
        Style.HoveredSlateSound.SetResourceObject(Hover);
    }
    Close->SetStyle(Style);
    Close->SetBackgroundColor(FLinearColor::White);
    Close->OnClicked.AddDynamic(this, &UVolumeSettingsWidget::OnCloseClicked);

    USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    CloseSize->SetWidthOverride(200.f);
    CloseSize->SetHeightOverride(54.f);
    CloseSize->SetContent(Close);

    UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseText"));
    CloseText->SetText(FText::FromString(TEXT("닫기")));
    CloseText->SetFont(MakeFont(true, 24, AccentDark, 2));
    CloseText->SetColorAndOpacity(FSlateColor(Hex(255, 250, 238)));
    Close->SetContent(CloseText);

    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(CloseSize))
    {
        S->SetPadding(FMargin(0.f, 26.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Center);
    }

    UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Hint"));
    Hint->SetText(FText::FromString(TEXT("ESC / F10 으로 열고 닫기")));
    Hint->SetFont(MakeFont(false, 14));
    Hint->SetColorAndOpacity(FSlateColor(LabelColor.CopyWithNewOpacity(0.6f)));
    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Hint))
    {
        S->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Center);
    }

    RefreshFromController();
}

UVolumeSettingsWidget::FVolumeRow UVolumeSettingsWidget::AddRow(UVerticalBox* Box, const FText& Label)
{
    FVolumeRow Row;

    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    // 이름
    USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LabelBox->SetWidthOverride(110.f);
    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    LabelText->SetText(Label);
    LabelText->SetFont(MakeFont(true, 22));
    LabelText->SetColorAndOpacity(FSlateColor(LabelColor));
    LabelBox->SetContent(LabelText);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(LabelBox))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    // 슬라이더: 채워지는 막대(ProgressBar) 위에 손잡이만 보이는 Slider를 겹친다
    USizeBox* SliderBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SliderBox->SetWidthOverride(340.f);
    SliderBox->SetHeightOverride(ThumbSize + 6.f);
    UOverlay* SliderStack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    SliderBox->SetContent(SliderStack);

    USizeBox* TrackSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    TrackSize->SetHeightOverride(16.f);
    Row.Fill = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
    FProgressBarStyle BarStyle = Row.Fill->GetWidgetStyle();
    BarStyle.SetBackgroundImage(RoundBox(TrackFill, 8.f, TrackLine, 2.f));
    BarStyle.SetFillImage(RoundBox(AccentColor, 8.f));
    Row.Fill->SetWidgetStyle(BarStyle);
    Row.Fill->SetFillColorAndOpacity(FLinearColor::White);
    Row.Fill->SetPercent(1.f);
    TrackSize->SetContent(Row.Fill);
    if (UOverlaySlot* S = SliderStack->AddChildToOverlay(TrackSize))
    {
        // 손잡이 중심이 움직이는 구간과 막대 끝을 맞춘다
        S->SetPadding(FMargin(ThumbSize * 0.5f, 0.f));
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Center);
    }

    Row.Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
    FSliderStyle SliderStyle = Row.Slider->GetWidgetStyle();
    const FSlateBrush NoBar = RoundBox(FLinearColor::Transparent, 0.f);
    SliderStyle.SetNormalBarImage(NoBar);
    SliderStyle.SetHoveredBarImage(NoBar);
    SliderStyle.SetDisabledBarImage(NoBar);
    const FVector2f Thumb(ThumbSize, ThumbSize);
    SliderStyle.SetNormalThumbImage(RoundBox(FLinearColor::White, ThumbSize * 0.5f, AccentColor, 4.f, Thumb));
    SliderStyle.SetHoveredThumbImage(RoundBox(Hex(255, 240, 236), ThumbSize * 0.5f, AccentDark, 4.f, Thumb));
    SliderStyle.SetDisabledThumbImage(RoundBox(Hex(230, 230, 230), ThumbSize * 0.5f, TrackLine, 4.f, Thumb));
    SliderStyle.SetBarThickness(16.f);
    Row.Slider->SetWidgetStyle(SliderStyle);
    Row.Slider->SetSliderBarColor(FLinearColor::White);
    Row.Slider->SetSliderHandleColor(FLinearColor::White);
    Row.Slider->SetMinValue(0.f);
    Row.Slider->SetMaxValue(1.f);
    Row.Slider->SetStepSize(0.05f);
    Row.Slider->SetValue(1.f);
    if (UOverlaySlot* S = SliderStack->AddChildToOverlay(Row.Slider))
    {
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(SliderBox))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    // 퍼센트
    USizeBox* PercentBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    PercentBox->SetWidthOverride(90.f);
    Row.Percent = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Row.Percent->SetJustification(ETextJustify::Right);
    Row.Percent->SetFont(MakeFont(true, 22));
    Row.Percent->SetColorAndOpacity(FSlateColor(AccentDark));
    Row.Percent->SetText(FText::FromString(TEXT("100%")));
    PercentBox->SetContent(Row.Percent);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(PercentBox))
    {
        // 슬라이더와 숫자 사이 간격
        S->SetPadding(FMargin(24.f, 0.f, 0.f, 0.f));
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Line))
    {
        S->SetPadding(FMargin(0.f, 8.f));
    }

    return Row;
}

void UVolumeSettingsWidget::SetRowValue(int32 Index, float Value)
{
    if (!Sliders.IsValidIndex(Index))
    {
        return;
    }

    Fills[Index]->SetPercent(Value);
    Percents[Index]->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f))));
}

void UVolumeSettingsWidget::RefreshFromController()
{
    if (!OwnerPC || Sliders.Num() < 3)
    {
        return;
    }

    const EVolumeChannel Channels[] = { EVolumeChannel::Master, EVolumeChannel::Music, EVolumeChannel::SFX };
    for (int32 i = 0; i < 3; ++i)
    {
        const float Value = OwnerPC->GetVolumeSetting(Channels[i]);
        Sliders[i]->SetValue(Value);
        SetRowValue(i, Value);
    }
}

void UVolumeSettingsWidget::HandleChanged(int32 Index, float Value)
{
    SetRowValue(Index, Value);

    if (OwnerPC)
    {
        const EVolumeChannel Channels[] = { EVolumeChannel::Master, EVolumeChannel::Music, EVolumeChannel::SFX };
        OwnerPC->SetVolumeSetting(Channels[Index], Value);
    }
}

void UVolumeSettingsWidget::OnMasterChanged(float Value) { HandleChanged(0, Value); }
void UVolumeSettingsWidget::OnMusicChanged(float Value) { HandleChanged(1, Value); }
void UVolumeSettingsWidget::OnSFXChanged(float Value) { HandleChanged(2, Value); }

void UVolumeSettingsWidget::OnCloseClicked()
{
    if (OwnerPC)
    {
        OwnerPC->CloseSettingsMenu();
    }
}

FReply UVolumeSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // 창에 포커스가 있으면 게임 쪽 키 입력이 안 들어오므로 여기서 닫는다
    if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::F10)
    {
        OnCloseClicked();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
