#include "SettingsWidget.h"

#include "MyPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
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
#include "Components/WidgetSwitcher.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Scalability.h"
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
    const FLinearColor AccentHover = Hex(255, 158, 150);
    const FLinearColor AccentDark = Hex(214, 82, 90);
    const FLinearColor TrackFill = Hex(244, 228, 198);
    const FLinearColor TrackLine = Hex(218, 180, 120);
    const FLinearColor TabIdleFill = Hex(255, 244, 222);
    const FLinearColor TabIdleHover = Hex(255, 232, 200);
    const FLinearColor ListHover = Hex(255, 226, 206);
    const FLinearColor ListSelected = Hex(255, 206, 196);

    FSlateBrush RoundBox(const FLinearColor& Fill, float Radius, const FLinearColor& Line = FLinearColor::Transparent, float LineWidth = 0.f, const FVector2f& Size = FVector2f::ZeroVector)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth, Size);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }

    constexpr float ThumbSize = 28.f;
    constexpr float LabelWidth = 200.f;     // "안티에일리어싱"까지 들어가는 폭
    constexpr float LabelGap = 12.f;        // 이름과 조작 칸 사이
    constexpr float ControlWidth = 340.f;   // 슬라이더 폭
    constexpr float ValueWidth = 90.f;      // 슬라이더 오른쪽 퍼센트 칸
    constexpr float ValueGap = 24.f;
    constexpr float SelectorWidth = ControlWidth + ValueGap + ValueWidth;   // < > / 드롭다운 폭(슬라이더 줄과 끝을 맞춤)
    constexpr float RowHeight = 38.f;
    constexpr float ConfirmSeconds = 10.f;

    const EWindowMode::Type WindowModes[] = { EWindowMode::Fullscreen, EWindowMode::WindowedFullscreen, EWindowMode::Windowed };
    const TCHAR* WindowModeNames[] = { TEXT("전체 화면"), TEXT("창 전체 화면"), TEXT("창 모드") };
    constexpr int32 NumWindowModes = UE_ARRAY_COUNT(WindowModes);

    const TCHAR* QualityNames[] = { TEXT("낮음"), TEXT("중간"), TEXT("높음"), TEXT("최고"), TEXT("시네마틱") };
    constexpr int32 MaxSelectableQuality = 3;   // 선택지는 낮음~최고(시네마틱은 표시만)

    const float FrameLimits[] = { 30.f, 60.f, 120.f, 144.f, 165.f, 240.f, 0.f };   // 0 = 무제한
    constexpr int32 NumFrameLimits = UE_ARRAY_COUNT(FrameLimits);

    FString QualityName(int32 Level)
    {
        if (Level < 0)
        {
            return TEXT("사용자 지정");
        }
        return QualityNames[FMath::Clamp(Level, 0, 4)];
    }

    FString FrameLimitName(float Limit)
    {
        return Limit <= 0.f ? FString(TEXT("무제한")) : FString::Printf(TEXT("%d FPS"), FMath::RoundToInt(Limit));
    }

    FString ResolutionName(const FIntPoint& R)
    {
        return FString::Printf(TEXT("%d x %d"), R.X, R.Y);
    }

    UGameUserSettings* GetSettings()
    {
        return GEngine ? GEngine->GetGameUserSettings() : nullptr;
    }

    // 렌더링 배율(%) - 설정 파일 값이 비어 있으면(에디터 등) 실제 엔진 값을 쓴다
    float GetEffectiveResolutionScale(const UGameUserSettings* Settings, float& OutMin, float& OutMax)
    {
        float Normalized = 1.f, Value = 100.f;
        OutMin = Scalability::MinResolutionScale;
        OutMax = Scalability::MaxResolutionScale;
        if (Settings)
        {
            Settings->GetResolutionScaleInformationEx(Normalized, Value, OutMin, OutMax);
        }
        if (Value < OutMin)
        {
            Value = Scalability::GetQualityLevels().ResolutionQuality;
        }
        if (Value < OutMin)
        {
            Value = 100.f;
        }
        return FMath::Clamp(Value, OutMin, OutMax);
    }
}

int32 USettingsWidget::LastTab = 0;

void USettingsComboHandler::HandleSelection(FString Item, ESelectInfo::Type SelectionType)
{
    // 코드로 값을 맞출 때(Direct)는 무시하고, 사람이 고른 것만 적용한다
    if (SelectionType == ESelectInfo::Direct)
    {
        return;
    }

    if (USettingsWidget* Widget = Owner.Get())
    {
        Widget->SelectOption(OptionIndex, Item);
    }
}

void USettingsWidget::InitFromController(AMyPlayerController* InPC)
{
    OwnerPC = InPC;
    RefreshAudio();
    RefreshGraphics();
}

TSharedRef<SWidget> USettingsWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

FSlateFontInfo USettingsWidget::MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor, int32 OutlineSize) const
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

UButton* USettingsWidget::MakePillButton(const FLinearColor& Fill, const FLinearColor& Hover, const FLinearColor& Pressed, float Radius)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    FButtonStyle Style = Button->GetStyle();
    Style.SetNormal(RoundBox(Fill, Radius, FLinearColor::White, 3.f));
    Style.SetHovered(RoundBox(Hover, Radius, FLinearColor::White, 3.f));
    Style.SetPressed(RoundBox(Pressed, Radius, FLinearColor::White, 3.f));
    Style.SetNormalPadding(FMargin(0.f));
    Style.SetPressedPadding(FMargin(0.f, 2.f, 0.f, 0.f));
    if (ClickSound)
    {
        Style.PressedSlateSound.SetResourceObject(ClickSound);
    }
    if (HoverSound)
    {
        Style.HoveredSlateSound.SetResourceObject(HoverSound);
    }
    Button->SetStyle(Style);
    Button->SetBackgroundColor(FLinearColor::White);
    return Button;
}

UButton* USettingsWidget::MakeTextButton(const FString& Label, const FLinearColor& Fill, const FLinearColor& Hover, const FLinearColor& Pressed, const FLinearColor& TextOutline, float Width, float Height, int32 FontSize)
{
    UButton* Button = MakePillButton(Fill, Hover, Pressed, Height * 0.5f);

    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(FText::FromString(Label));
    Text->SetFont(MakeFont(true, FontSize, TextOutline, 2));
    Text->SetColorAndOpacity(FSlateColor(Hex(255, 250, 238)));
    Button->SetContent(Text);

    return Button;
}

void USettingsWidget::BuildTree()
{
    SetIsFocusable(true);

    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    LightFont = LoadObject<UObject>(nullptr, LightFontPath);
    ClickSound = LoadObject<USoundBase>(nullptr, ClickSoundPath);
    HoverSound = LoadObject<USoundBase>(nullptr, HoverSoundPath);

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
    Inner->SetPadding(FMargin(44.f, 46.f, 44.f, 24.f));
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
    Title->SetText(FText::FromString(TEXT("설정")));
    Title->SetFont(MakeFont(true, 30, RibbonLine, 3));
    Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Ribbon->SetContent(Title);

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Body"));
    Inner->SetContent(Box);

    // ----- 탭 -----
    UHorizontalBox* TabBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TabBar"));
    const FString TabNames[] = { TEXT("그래픽"), TEXT("오디오") };
    for (int32 i = 0; i < 2; ++i)
    {
        UTextBlock* TabText = nullptr;
        UButton* Tab = MakeTabButton(TabNames[i], TabText);
        TabButtons.Add(Tab);
        TabTexts.Add(TabText);

        USizeBox* TabSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        TabSize->SetWidthOverride(170.f);
        TabSize->SetHeightOverride(48.f);
        TabSize->SetContent(Tab);
        if (UHorizontalBoxSlot* S = TabBar->AddChildToHorizontalBox(TabSize))
        {
            S->SetPadding(FMargin(i == 0 ? 0.f : 8.f, 0.f, i == 0 ? 8.f : 0.f, 0.f));
        }
    }
    TabButtons[0]->OnClicked.AddDynamic(this, &USettingsWidget::OnGraphicsTabClicked);
    TabButtons[1]->OnClicked.AddDynamic(this, &USettingsWidget::OnAudioTabClicked);
    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(TabBar))
    {
        S->SetHorizontalAlignment(HAlign_Center);
    }

    // 탭 아래 얇은 구분선
    UBorder* Divider = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Divider"));
    Divider->SetBrush(RoundBox(PanelInnerLine, 1.f));
    USizeBox* DividerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    DividerSize->SetHeightOverride(2.f);
    DividerSize->SetContent(Divider);
    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(DividerSize))
    {
        S->SetPadding(FMargin(0.f, 16.f, 0.f, 10.f));
        S->SetHorizontalAlignment(HAlign_Fill);
    }

    // ----- 페이지 -----
    Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Pages"));
    Box->AddChildToVerticalBox(Pages);

    UVerticalBox* GraphicsPage = MakePage(TEXT("GraphicsPage"));
    UVerticalBox* AudioPage = MakePage(TEXT("AudioPage"));

    OptionCombos.SetNum(static_cast<int32>(EOption::Count));

    // 오디오 (슬라이더 0~2)
    AddSectionTitle(AudioPage, FText::FromString(TEXT("음량")), true);
    const FText AudioLabels[] = {
        FText::FromString(TEXT("전체")),
        FText::FromString(TEXT("배경음")),
        FText::FromString(TEXT("효과음"))
    };
    for (const FText& Label : AudioLabels)
    {
        const FSliderRow Row = AddSliderRow(AudioPage, Label);
        Sliders.Add(Row.Slider);
        Fills.Add(Row.Fill);
        Percents.Add(Row.Percent);
    }
    Sliders[0]->OnValueChanged.AddDynamic(this, &USettingsWidget::OnMasterChanged);
    Sliders[1]->OnValueChanged.AddDynamic(this, &USettingsWidget::OnMusicChanged);
    Sliders[2]->OnValueChanged.AddDynamic(this, &USettingsWidget::OnSFXChanged);

    // 그래픽 (여러 값 중 고르는 건 드롭다운, 켬/끔은 체크박스)
    AddSectionTitle(GraphicsPage, FText::FromString(TEXT("화면")), true);
    AddComboRow(GraphicsPage, EOption::WindowMode, FText::FromString(TEXT("화면 모드")));
    AddComboRow(GraphicsPage, EOption::Resolution, FText::FromString(TEXT("해상도")));
    {
        const FSliderRow Row = AddSliderRow(GraphicsPage, FText::FromString(TEXT("렌더링 배율")));
        Sliders.Add(Row.Slider);   // 슬라이더 3
        Fills.Add(Row.Fill);
        Percents.Add(Row.Percent);
        Row.Slider->SetStepSize(0.02f);
        Row.Slider->OnValueChanged.AddDynamic(this, &USettingsWidget::OnRenderScaleChanged);
    }

    AddSectionTitle(GraphicsPage, FText::FromString(TEXT("품질")), false);
    AddComboRow(GraphicsPage, EOption::OverallQuality, FText::FromString(TEXT("전체 품질")));
    AddComboRow(GraphicsPage, EOption::ShadowQuality, FText::FromString(TEXT("그림자")));
    AddComboRow(GraphicsPage, EOption::TextureQuality, FText::FromString(TEXT("텍스처")));
    AddComboRow(GraphicsPage, EOption::AntiAliasing, FText::FromString(TEXT("안티에일리어싱")));

    AddSectionTitle(GraphicsPage, FText::FromString(TEXT("성능")), false);
    AddComboRow(GraphicsPage, EOption::FrameLimit, FText::FromString(TEXT("최대 프레임")));
    {
        UTextBlock* StateText = nullptr;
        VSyncCheck = AddCheckRow(GraphicsPage, FText::FromString(TEXT("수직 동기화")), StateText);
        VSyncStateText = StateText;
        VSyncCheck->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnVSyncChanged);
    }

    // ----- 변경 확인 띠 -----
    BuildConfirmBanner(Box);

    // ----- 닫기 버튼: START 버튼 같은 코랄 알약 모양 -----
    UButton* Close = MakeTextButton(TEXT("닫기"), AccentColor, AccentHover, AccentDark, AccentDark, 200.f, 54.f, 24);
    Close->OnClicked.AddDynamic(this, &USettingsWidget::OnCloseClicked);

    USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    CloseSize->SetWidthOverride(200.f);
    CloseSize->SetHeightOverride(54.f);
    CloseSize->SetContent(Close);
    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(CloseSize))
    {
        S->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Center);
    }

    UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Hint"));
    Hint->SetText(FText::FromString(TEXT("ESC / F10 으로 열고 닫기 · Q / E 로 탭 이동")));
    Hint->SetFont(MakeFont(false, 14));
    Hint->SetColorAndOpacity(FSlateColor(LabelColor.CopyWithNewOpacity(0.6f)));
    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Hint))
    {
        S->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Center);
    }

    RefreshAudio();
    RefreshGraphics();
    UpdateConfirmBanner();
    SelectTab(LastTab);
}

void USettingsWidget::BuildConfirmBanner(UVerticalBox* Box)
{
    // 높이를 늘 차지해 두고 보였다 숨겼다만 한다(나타날 때 창이 움직여 다른 버튼을 잘못 누르지 않게)
    ConfirmBanner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ConfirmBanner"));
    ConfirmBanner->SetBrush(RoundBox(Hex(255, 240, 214), 16.f, PanelOuterLine, 2.f));
    ConfirmBanner->SetPadding(FMargin(18.f, 8.f, 10.f, 8.f));

    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    ConfirmBanner->SetContent(Line);

    // 안내 글은 두 줄로 고정(제목 + 남은 초). 글이 바뀌어도 크기가 변하지 않게 줄바꿈은 쓰지 않는다.
    UVerticalBox* TextStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UTextBlock* ConfirmTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmTitle"));
    ConfirmTitle->SetText(FText::FromString(TEXT("바뀐 설정을 유지할까요?")));
    ConfirmTitle->SetFont(MakeFont(true, 17));
    ConfirmTitle->SetColorAndOpacity(FSlateColor(LabelColor));
    TextStack->AddChildToVerticalBox(ConfirmTitle);

    ConfirmText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmText"));
    ConfirmText->SetText(FText::FromString(TEXT("10초 후 원래대로 돌아갑니다")));
    ConfirmText->SetFont(MakeFont(false, 14));
    ConfirmText->SetColorAndOpacity(FSlateColor(AccentDark));
    TextStack->AddChildToVerticalBox(ConfirmText);

    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(TextStack))
    {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetVerticalAlignment(VAlign_Center);
    }

    UButton* Keep = MakeTextButton(TEXT("유지"), AccentColor, AccentHover, AccentDark, AccentDark, 110.f, 40.f, 18);
    Keep->OnClicked.AddDynamic(this, &USettingsWidget::OnKeepClicked);
    USizeBox* KeepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    KeepSize->SetWidthOverride(110.f);
    KeepSize->SetHeightOverride(40.f);
    KeepSize->SetContent(Keep);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(KeepSize))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
    }

    UButton* Revert = MakeTextButton(TEXT("되돌리기"), Hex(196, 160, 120), Hex(214, 180, 140), Hex(170, 132, 94), Hex(140, 100, 60), 130.f, 40.f, 18);
    Revert->OnClicked.AddDynamic(this, &USettingsWidget::OnRevertClicked);
    USizeBox* RevertSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    RevertSize->SetWidthOverride(130.f);
    RevertSize->SetHeightOverride(40.f);
    RevertSize->SetContent(Revert);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(RevertSize))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
    }

    // 띠 크기를 고정: 폭은 설정 줄 전체 폭(이름 + 조작 칸), 높이는 두 줄 글 + 버튼이 들어가는 만큼.
    // 숨겨져 있을 때도 같은 자리를 차지하므로 띠가 나타나도 창 크기가 바뀌지 않는다.
    USizeBox* BannerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ConfirmBannerSize"));
    BannerSize->SetWidthOverride(LabelWidth + LabelGap + SelectorWidth);
    BannerSize->SetHeightOverride(64.f);
    BannerSize->SetContent(ConfirmBanner);

    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(BannerSize))
    {
        S->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Center);
    }
}

UVerticalBox* USettingsWidget::MakePage(const TCHAR* Name)
{
    UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
    Pages->AddChild(Page);
    return Page;
}

UButton* USettingsWidget::MakeTabButton(const FString& Label, UTextBlock*& OutText)
{
    UButton* Tab = MakePillButton(TabIdleFill, TabIdleHover, TabIdleHover, 24.f);

    OutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    OutText->SetText(FText::FromString(Label));
    OutText->SetFont(MakeFont(true, 22));
    OutText->SetColorAndOpacity(FSlateColor(LabelColor));
    Tab->SetContent(OutText);
    return Tab;
}

void USettingsWidget::AddSectionTitle(UVerticalBox* Box, const FText& Title, bool bFirst)
{
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(Title);
    Text->SetFont(MakeFont(true, 16));
    Text->SetColorAndOpacity(FSlateColor(AccentDark));
    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Text))
    {
        S->SetPadding(FMargin(2.f, bFirst ? 4.f : 14.f, 0.f, 2.f));
    }
}

UWidget* USettingsWidget::MakeRowLabel(const FText& Label)
{
    // 이름 칸은 폭을 고정하고, 넘치더라도 옆 칸을 침범하지 않게 잘라낸다
    USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LabelBox->SetWidthOverride(LabelWidth);
    LabelBox->SetClipping(EWidgetClipping::ClipToBounds);

    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    LabelText->SetText(Label);
    LabelText->SetFont(MakeFont(true, 20));
    LabelText->SetColorAndOpacity(FSlateColor(LabelColor));
    LabelBox->SetContent(LabelText);
    return LabelBox;
}

USettingsWidget::FSliderRow USettingsWidget::AddSliderRow(UVerticalBox* Box, const FText& Label)
{
    FSliderRow Row;

    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(MakeRowLabel(Label)))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(0.f, 0.f, LabelGap, 0.f));
    }

    // 슬라이더: 채워지는 막대(ProgressBar) 위에 손잡이만 보이는 Slider를 겹친다
    USizeBox* SliderBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SliderBox->SetWidthOverride(ControlWidth);
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
    PercentBox->SetWidthOverride(ValueWidth);
    Row.Percent = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Row.Percent->SetJustification(ETextJustify::Right);
    Row.Percent->SetFont(MakeFont(true, 20));
    Row.Percent->SetColorAndOpacity(FSlateColor(AccentDark));
    Row.Percent->SetText(FText::FromString(TEXT("100%")));
    PercentBox->SetContent(Row.Percent);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(PercentBox))
    {
        // 슬라이더와 숫자 사이 간격
        S->SetPadding(FMargin(ValueGap, 0.f, 0.f, 0.f));
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Line))
    {
        S->SetPadding(FMargin(0.f, 5.f));
    }

    return Row;
}

UCheckBox* USettingsWidget::AddCheckRow(UVerticalBox* Box, const FText& Label, UTextBlock*& OutStateText)
{
    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(MakeRowLabel(Label)))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(0.f, 0.f, LabelGap, 0.f));
    }

    UCheckBox* Check = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());

    // 크림색 둥근 네모 + 체크하면 코랄 네모 안에 흰 체크 표시(엔진 기본 체크 아이콘을 색만 바꿔 사용)
    constexpr float BoxSize = 32.f;
    const FVector2f BoxVec(BoxSize, BoxSize);
    FCheckBoxStyle Style = Check->GetWidgetStyle();
    FSlateBrush Mark = Style.CheckedImage;
    Mark.ImageSize = FVector2D(22.f, 22.f);
    Mark.TintColor = FSlateColor(FLinearColor::White);
    FSlateBrush NoMark = Mark;
    NoMark.TintColor = FSlateColor(FLinearColor::Transparent);

    Style.SetCheckBoxType(ESlateCheckBoxType::CheckBox);
    Style.SetUncheckedImage(NoMark);
    Style.SetUncheckedHoveredImage(NoMark);
    Style.SetUncheckedPressedImage(NoMark);
    Style.SetCheckedImage(Mark);
    Style.SetCheckedHoveredImage(Mark);
    Style.SetCheckedPressedImage(Mark);
    Style.SetBackgroundImage(RoundBox(TrackFill, 8.f, TrackLine, 2.f, BoxVec));
    Style.SetBackgroundHoveredImage(RoundBox(Hex(250, 236, 210), 8.f, PanelOuterLine, 2.f, BoxVec));
    Style.SetBackgroundPressedImage(RoundBox(Hex(250, 236, 210), 8.f, AccentDark, 2.f, BoxVec));
    Style.SetForegroundColor(FSlateColor(FLinearColor::White));
    Style.SetHoveredForegroundColor(FSlateColor(FLinearColor::White));
    Style.SetPressedForegroundColor(FSlateColor(FLinearColor::White));
    Style.SetCheckedForegroundColor(FSlateColor(FLinearColor::White));
    Style.SetCheckedHoveredForegroundColor(FSlateColor(FLinearColor::White));
    Style.SetCheckedPressedForegroundColor(FSlateColor(FLinearColor::White));
    Style.SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
    if (ClickSound)
    {
        FSlateSound Click;
        Click.SetResourceObject(ClickSound);
        Style.SetCheckedSound(Click);
        Style.SetUncheckedSound(Click);
    }
    if (HoverSound)
    {
        FSlateSound Hover;
        Hover.SetResourceObject(HoverSound);
        Style.SetHoveredSound(Hover);
    }
    Check->SetWidgetStyle(Style);

    // 체크박스 옆 상태 글자(켬/끔). 글자를 눌러도 체크되도록 체크박스 내용으로 넣는다.
    OutStateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    OutStateText->SetFont(MakeFont(true, 18));
    OutStateText->SetColorAndOpacity(FSlateColor(LabelColor));
    Check->SetContent(OutStateText);

    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Check))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Line))
    {
        S->SetPadding(FMargin(0.f, 7.f));
    }

    return Check;
}

void USettingsWidget::SetCheckRowState(UCheckBox* Check, UTextBlock* StateText, bool bOn)
{
    if (!Check || !StateText)
    {
        return;
    }

    // 체크하면 바탕을 코랄로 칠해 흰 체크 표시가 보이게 한다
    FCheckBoxStyle Style = Check->GetWidgetStyle();
    const FVector2f BoxVec(32.f, 32.f);
    if (bOn)
    {
        Style.SetBackgroundImage(RoundBox(AccentColor, 8.f, AccentDark, 2.f, BoxVec));
        Style.SetBackgroundHoveredImage(RoundBox(AccentHover, 8.f, AccentDark, 2.f, BoxVec));
        Style.SetBackgroundPressedImage(RoundBox(AccentDark, 8.f, AccentDark, 2.f, BoxVec));
    }
    else
    {
        Style.SetBackgroundImage(RoundBox(TrackFill, 8.f, TrackLine, 2.f, BoxVec));
        Style.SetBackgroundHoveredImage(RoundBox(Hex(250, 236, 210), 8.f, PanelOuterLine, 2.f, BoxVec));
        Style.SetBackgroundPressedImage(RoundBox(Hex(250, 236, 210), 8.f, AccentDark, 2.f, BoxVec));
    }
    Check->SetWidgetStyle(Style);

    bRefreshing = true;
    Check->SetIsChecked(bOn);
    bRefreshing = false;

    StateText->SetText(FText::FromString(bOn ? TEXT("켬") : TEXT("끔")));
    StateText->SetColorAndOpacity(FSlateColor(bOn ? AccentDark : LabelColor.CopyWithNewOpacity(0.7f)));
}

void USettingsWidget::AddComboRow(UVerticalBox* Box, EOption Option, const FText& Label)
{
    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(MakeRowLabel(Label)))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(0.f, 0.f, LabelGap, 0.f));
    }

    UComboBoxString* Combo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());

    // 닫힌 상태: < > 줄의 값 상자와 같은 크림색 둥근 상자 + 아래 화살표
    FComboBoxStyle ComboStyle = Combo->GetWidgetStyle();
    FComboButtonStyle ButtonPart = ComboStyle.ComboButtonStyle;
    FButtonStyle Button = ButtonPart.ButtonStyle;
    Button.SetNormal(RoundBox(TrackFill, 12.f, TrackLine, 2.f));
    Button.SetHovered(RoundBox(Hex(250, 236, 210), 12.f, PanelOuterLine, 2.f));
    Button.SetPressed(RoundBox(Hex(250, 236, 210), 12.f, AccentDark, 2.f));
    Button.SetNormalPadding(FMargin(0.f));
    Button.SetPressedPadding(FMargin(0.f));
    if (HoverSound)
    {
        Button.HoveredSlateSound.SetResourceObject(HoverSound);
    }
    ButtonPart.SetButtonStyle(Button);
    ButtonPart.SetMenuBorderBrush(RoundBox(PanelInnerFill, 12.f, PanelOuterLine, 2.f));
    ButtonPart.SetMenuBorderPadding(FMargin(4.f));
    ButtonPart.SetDownArrowPadding(FMargin(0.f, 0.f, 12.f, 0.f));
    ComboStyle.SetComboButtonStyle(ButtonPart);
    ComboStyle.SetContentPadding(FMargin(10.f, 2.f, 4.f, 2.f));
    ComboStyle.SetMenuRowPadding(FMargin(0.f));
    if (ClickSound)
    {
        FSlateSound Click;
        Click.SetResourceObject(ClickSound);
        ComboStyle.SetPressedSound(Click);
        ComboStyle.SetSelectionChangeSound(Click);
    }
    Combo->SetWidgetStyle(ComboStyle);

    // 펼친 목록: 크림색 줄, 마우스 올리면 연한 살구, 선택된 줄은 연한 코랄
    FTableRowStyle RowStyle = Combo->GetItemStyle();
    const FSlateBrush RowFill = RoundBox(PanelInnerFill, 8.f);
    const FSlateBrush RowHover = RoundBox(ListHover, 8.f);
    const FSlateBrush RowSelected = RoundBox(ListSelected, 8.f);
    RowStyle.SetEvenRowBackgroundBrush(RowFill);
    RowStyle.SetOddRowBackgroundBrush(RowFill);
    RowStyle.SetEvenRowBackgroundHoveredBrush(RowHover);
    RowStyle.SetOddRowBackgroundHoveredBrush(RowHover);
    RowStyle.SetActiveBrush(RowSelected);
    RowStyle.SetActiveHoveredBrush(RowSelected);
    RowStyle.SetInactiveBrush(RowSelected);
    RowStyle.SetInactiveHoveredBrush(RowSelected);
    RowStyle.SetActiveHighlightedBrush(RowSelected);
    RowStyle.SetInactiveHighlightedBrush(RowSelected);
    RowStyle.SetSelectorFocusedBrush(RoundBox(FLinearColor::Transparent, 8.f));
    RowStyle.SetTextColor(FSlateColor(LabelColor));
    RowStyle.SetSelectedTextColor(FSlateColor(LabelColor));
    Combo->SetItemStyle(RowStyle);
    Combo->SetMaxListHeight(280.f);

    // 항목 글자는 메이플스토리 폰트로 직접 만든다(닫힌 상태의 표시도 같은 함수 사용)
    Combo->OnGenerateWidgetEvent.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(USettingsWidget, GenerateComboItem));

    // 선택 결과를 "몇 번째 설정인지"와 함께 받는다. 항목 목록은 RefreshCombo에서 채운다.
    USettingsComboHandler* Handler = NewObject<USettingsComboHandler>(this);
    Handler->Owner = this;
    Handler->OptionIndex = static_cast<int32>(Option);
    Combo->OnSelectionChanged.AddDynamic(Handler, &USettingsComboHandler::HandleSelection);
    ComboHandlers.Add(Handler);

    OptionCombos[static_cast<int32>(Option)] = Combo;

    USizeBox* ComboSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    ComboSize->SetWidthOverride(SelectorWidth);
    ComboSize->SetHeightOverride(RowHeight);
    ComboSize->SetContent(Combo);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(ComboSize))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Line))
    {
        S->SetPadding(FMargin(0.f, 5.f));
    }
}

UWidget* USettingsWidget::GenerateComboItem(FString Item)
{
    UBorder* Cell = NewObject<UBorder>(this);
    Cell->SetBrush(RoundBox(FLinearColor::Transparent, 0.f));
    Cell->SetHorizontalAlignment(HAlign_Center);
    Cell->SetVerticalAlignment(VAlign_Center);
    Cell->SetPadding(FMargin(8.f, 5.f, 8.f, 6.f));

    UTextBlock* Text = NewObject<UTextBlock>(this);
    Text->SetText(FText::FromString(Item));
    Text->SetFont(MakeFont(true, 18));
    Text->SetColorAndOpacity(FSlateColor(LabelColor));
    Text->SetJustification(ETextJustify::Center);
    Cell->SetContent(Text);
    return Cell;
}

void USettingsWidget::SelectTab(int32 TabIndex)
{
    ActiveTab = FMath::Clamp(TabIndex, 0, TabButtons.Num() - 1);
    LastTab = ActiveTab;

    if (Pages)
    {
        Pages->SetActiveWidgetIndex(ActiveTab);
    }

    for (int32 i = 0; i < TabButtons.Num(); ++i)
    {
        const bool bActive = (i == ActiveTab);
        FButtonStyle Style = TabButtons[i]->GetStyle();
        if (bActive)
        {
            Style.SetNormal(RoundBox(AccentColor, 24.f, FLinearColor::White, 3.f));
            Style.SetHovered(RoundBox(AccentHover, 24.f, FLinearColor::White, 3.f));
            Style.SetPressed(RoundBox(AccentDark, 24.f, FLinearColor::White, 3.f));
            TabTexts[i]->SetFont(MakeFont(true, 22, AccentDark, 2));
            TabTexts[i]->SetColorAndOpacity(FSlateColor(Hex(255, 250, 238)));
        }
        else
        {
            Style.SetNormal(RoundBox(TabIdleFill, 24.f, TrackLine, 2.f));
            Style.SetHovered(RoundBox(TabIdleHover, 24.f, PanelOuterLine, 2.f));
            Style.SetPressed(RoundBox(TabIdleHover, 24.f, PanelOuterLine, 2.f));
            TabTexts[i]->SetFont(MakeFont(true, 22));
            TabTexts[i]->SetColorAndOpacity(FSlateColor(LabelColor.CopyWithNewOpacity(0.75f)));
        }
        TabButtons[i]->SetStyle(Style);
    }
}

void USettingsWidget::OnGraphicsTabClicked() { SelectTab(0); }
void USettingsWidget::OnAudioTabClicked() { SelectTab(1); }

void USettingsWidget::SetSliderRowValue(int32 Index, float Value)
{
    if (!Fills.IsValidIndex(Index))
    {
        return;
    }

    Fills[Index]->SetPercent(Value);
    Percents[Index]->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f))));
}

// ===== 오디오 =====

void USettingsWidget::RefreshAudio()
{
    if (!OwnerPC || Sliders.Num() < 3)
    {
        return;
    }

    bRefreshing = true;
    const EVolumeChannel Channels[] = { EVolumeChannel::Master, EVolumeChannel::Music, EVolumeChannel::SFX };
    for (int32 i = 0; i < 3; ++i)
    {
        const float Value = OwnerPC->GetVolumeSetting(Channels[i]);
        Sliders[i]->SetValue(Value);
        SetSliderRowValue(i, Value);
    }
    bRefreshing = false;
}

void USettingsWidget::OnMasterChanged(float Value)
{
    SetSliderRowValue(0, Value);
    if (OwnerPC && !bRefreshing) OwnerPC->SetVolumeSetting(EVolumeChannel::Master, Value);
}

void USettingsWidget::OnMusicChanged(float Value)
{
    SetSliderRowValue(1, Value);
    if (OwnerPC && !bRefreshing) OwnerPC->SetVolumeSetting(EVolumeChannel::Music, Value);
}

void USettingsWidget::OnSFXChanged(float Value)
{
    SetSliderRowValue(2, Value);
    if (OwnerPC && !bRefreshing) OwnerPC->SetVolumeSetting(EVolumeChannel::SFX, Value);
}

// ===== 그래픽: 화면 표시 =====

void USettingsWidget::RefreshGraphics()
{
    UGameUserSettings* Settings = GetSettings();
    if (!Settings || OptionCombos.Num() < static_cast<int32>(EOption::Count))
    {
        return;
    }

    // 해상도 선택지: 모니터가 지원하는 해상도 + 현재 해상도
    Resolutions.Reset();
    TArray<FIntPoint> Supported;
    UKismetSystemLibrary::GetSupportedFullscreenResolutions(Supported);
    for (const FIntPoint& R : Supported)
    {
        Resolutions.AddUnique(R);
    }
    Resolutions.AddUnique(Settings->GetScreenResolution());
    Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
    {
        return A.X != B.X ? A.X < B.X : A.Y < B.Y;
    });

    for (int32 i = 0; i < static_cast<int32>(EOption::Count); ++i)
    {
        RefreshCombo(static_cast<EOption>(i));
    }

    SetCheckRowState(VSyncCheck, VSyncStateText, Settings->IsVSyncEnabled());

    // 렌더링 배율(화면 해상도 대비 실제로 그리는 비율)
    if (Sliders.IsValidIndex(3))
    {
        float Min = 0.f, Max = 100.f;
        const float Scale = GetEffectiveResolutionScale(Settings, Min, Max);
        const float Normalized = Max > Min ? (Scale - Min) / (Max - Min) : 1.f;

        bRefreshing = true;
        Sliders[3]->SetValue(Normalized);
        bRefreshing = false;

        Fills[3]->SetPercent(Normalized);
        Percents[3]->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Scale))));
    }
}

void USettingsWidget::RefreshCombo(EOption Option)
{
    UComboBoxString* Combo = OptionCombos[static_cast<int32>(Option)];
    const UGameUserSettings* Settings = GetSettings();
    if (!Combo || !Settings)
    {
        return;
    }

    // 고를 수 있는 값 목록 + 지금 값. 지금 값이 목록에 없으면(사용자 지정, 시네마틱, ini에서 직접 바꾼 값) 그것도 넣어서 보여준다.
    TArray<FString> Items;
    FString Current;

    auto AddQualityItems = [&Items](int32 CurrentLevel)
    {
        for (int32 i = 0; i <= MaxSelectableQuality; ++i)
        {
            Items.Add(QualityNames[i]);
        }
        return QualityName(CurrentLevel);
    };

    switch (Option)
    {
    case EOption::WindowMode:
        for (int32 i = 0; i < NumWindowModes; ++i)
        {
            Items.Add(WindowModeNames[i]);
            if (WindowModes[i] == Settings->GetFullscreenMode())
            {
                Current = WindowModeNames[i];
            }
        }
        break;

    case EOption::Resolution:
        for (const FIntPoint& R : Resolutions)
        {
            Items.Add(ResolutionName(R));
        }
        Current = ResolutionName(Settings->GetScreenResolution());
        break;

    case EOption::OverallQuality: Current = AddQualityItems(Settings->GetOverallScalabilityLevel()); break;
    case EOption::ShadowQuality: Current = AddQualityItems(Settings->GetShadowQuality()); break;
    case EOption::TextureQuality: Current = AddQualityItems(Settings->GetTextureQuality()); break;
    case EOption::AntiAliasing: Current = AddQualityItems(Settings->GetAntiAliasingQuality()); break;

    case EOption::FrameLimit:
        for (const float Limit : FrameLimits)
        {
            Items.Add(FrameLimitName(Limit));
        }
        Current = FrameLimitName(Settings->GetFrameRateLimit());
        break;

    default:
        return;
    }

    if (!Current.IsEmpty())
    {
        Items.AddUnique(Current);
    }

    bRefreshing = true;
    Combo->ClearOptions();
    for (const FString& Item : Items)
    {
        Combo->AddOption(Item);
    }
    Combo->SetSelectedOption(Current);
    bRefreshing = false;
}

// ===== 그래픽: 값 바꾸기 =====

void USettingsWidget::SelectOption(int32 OptionIndex, const FString& Item)
{
    UGameUserSettings* Settings = GetSettings();
    if (bRefreshing || !Settings)
    {
        return;
    }

    // 품질 이름 -> 단계(낮음 0 ~ 시네마틱 4). "사용자 지정"은 고를 수 있는 값이 아니다.
    auto QualityFromName = [](const FString& Name) -> int32
    {
        for (int32 i = 0; i < UE_ARRAY_COUNT(QualityNames); ++i)
        {
            if (Name == QualityNames[i])
            {
                return i;
            }
        }
        return INDEX_NONE;
    };

    switch (static_cast<EOption>(OptionIndex))
    {
    case EOption::WindowMode:
        for (int32 i = 0; i < NumWindowModes; ++i)
        {
            if (Item == WindowModeNames[i] && WindowModes[i] != Settings->GetFullscreenMode())
            {
                BeginGraphicsChange();
                Settings->SetFullscreenMode(WindowModes[i]);
                ApplyGraphicsPreview();
                return;
            }
        }
        return;

    case EOption::Resolution:
        for (const FIntPoint& R : Resolutions)
        {
            if (Item == ResolutionName(R) && R != Settings->GetScreenResolution())
            {
                BeginGraphicsChange();
                Settings->SetScreenResolution(R);
                ApplyGraphicsPreview();
                return;
            }
        }
        return;

    case EOption::OverallQuality:
    case EOption::ShadowQuality:
    case EOption::TextureQuality:
    case EOption::AntiAliasing:
    {
        const int32 Level = QualityFromName(Item);
        if (Level == INDEX_NONE)
        {
            return;
        }

        BeginGraphicsChange();
        switch (static_cast<EOption>(OptionIndex))
        {
        case EOption::OverallQuality: Settings->SetOverallScalabilityLevel(Level); break;
        case EOption::ShadowQuality: Settings->SetShadowQuality(Level); break;
        case EOption::TextureQuality: Settings->SetTextureQuality(Level); break;
        case EOption::AntiAliasing: Settings->SetAntiAliasingQuality(Level); break;
        default: break;
        }
        ApplyGraphicsPreview();
        return;
    }

    case EOption::FrameLimit:
        for (const float Limit : FrameLimits)
        {
            if (Item == FrameLimitName(Limit) && !FMath::IsNearlyEqual(Limit, Settings->GetFrameRateLimit(), 0.5f))
            {
                BeginGraphicsChange();
                Settings->SetFrameRateLimit(Limit);
                ApplyGraphicsPreview();
                return;
            }
        }
        return;

    default:
        return;
    }
}

void USettingsWidget::OnVSyncChanged(bool bIsChecked)
{
    UGameUserSettings* Settings = GetSettings();
    if (bRefreshing || !Settings || Settings->IsVSyncEnabled() == bIsChecked)
    {
        return;
    }

    BeginGraphicsChange();
    Settings->SetVSyncEnabled(bIsChecked);
    ApplyGraphicsPreview();
}

void USettingsWidget::OnRenderScaleChanged(float Value)
{
    if (bRefreshing)
    {
        return;
    }

    UGameUserSettings* Settings = GetSettings();
    if (!Settings)
    {
        return;
    }

    BeginGraphicsChange();

    // 드래그 중에는 해상도/창은 건드리지 않고 화면 그리기 설정만 바로 반영
    Settings->SetResolutionScaleNormalized(Value);
    Settings->ApplyNonResolutionSettings();
    PendingRemain = ConfirmSeconds;

    float Min = 0.f, Max = 100.f;
    const float Scale = GetEffectiveResolutionScale(Settings, Min, Max);
    Fills[3]->SetPercent(Value);
    Percents[3]->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Scale))));
    UpdateConfirmBanner();
}

// ===== 그래픽: 적용 유예(10초 안에 유지를 누르지 않으면 되돌림) =====

USettingsWidget::FGraphicsSnapshot USettingsWidget::CaptureGraphics() const
{
    FGraphicsSnapshot Snap;
    const UGameUserSettings* Settings = GetSettings();
    if (!Settings)
    {
        return Snap;
    }

    float Min = 0.f, Max = 100.f;
    const float Scale = GetEffectiveResolutionScale(Settings, Min, Max);

    Snap.WindowMode = Settings->GetFullscreenMode();
    Snap.Resolution = Settings->GetScreenResolution();
    Snap.ResolutionScaleNormalized = Max > Min ? (Scale - Min) / (Max - Min) : 1.f;
    Snap.ViewDistance = Settings->GetViewDistanceQuality();
    Snap.Shadow = Settings->GetShadowQuality();
    Snap.GlobalIllumination = Settings->GetGlobalIlluminationQuality();
    Snap.Reflection = Settings->GetReflectionQuality();
    Snap.AntiAliasing = Settings->GetAntiAliasingQuality();
    Snap.Texture = Settings->GetTextureQuality();
    Snap.Effects = Settings->GetVisualEffectQuality();
    Snap.PostProcess = Settings->GetPostProcessingQuality();
    Snap.Foliage = Settings->GetFoliageQuality();
    Snap.Shading = Settings->GetShadingQuality();
    Snap.FrameLimit = Settings->GetFrameRateLimit();
    Snap.bVSync = Settings->IsVSyncEnabled();
    return Snap;
}

void USettingsWidget::RestoreGraphics(const FGraphicsSnapshot& Snap)
{
    UGameUserSettings* Settings = GetSettings();
    if (!Settings)
    {
        return;
    }

    Settings->SetFullscreenMode(Snap.WindowMode);
    Settings->SetScreenResolution(Snap.Resolution);
    Settings->SetViewDistanceQuality(Snap.ViewDistance);
    Settings->SetShadowQuality(Snap.Shadow);
    Settings->SetGlobalIlluminationQuality(Snap.GlobalIllumination);
    Settings->SetReflectionQuality(Snap.Reflection);
    Settings->SetAntiAliasingQuality(Snap.AntiAliasing);
    Settings->SetTextureQuality(Snap.Texture);
    Settings->SetVisualEffectQuality(Snap.Effects);
    Settings->SetPostProcessingQuality(Snap.PostProcess);
    Settings->SetFoliageQuality(Snap.Foliage);
    Settings->SetShadingQuality(Snap.Shading);
    // 해상도 배율은 해상도를 정한 뒤에 넣어야 맞게 계산된다
    Settings->SetResolutionScaleNormalized(Snap.ResolutionScaleNormalized);
    Settings->SetFrameRateLimit(Snap.FrameLimit);
    Settings->SetVSyncEnabled(Snap.bVSync);

    Settings->ApplyResolutionSettings(false);
    Settings->ApplyNonResolutionSettings();
}

void USettingsWidget::BeginGraphicsChange()
{
    // 확인 대기 중에 또 바꾸면 처음 상태를 그대로 두고 카운트다운만 다시 시작한다
    if (!bPendingConfirm)
    {
        PendingSnapshot = CaptureGraphics();
        bPendingConfirm = true;
    }
    PendingRemain = ConfirmSeconds;
    LastShownSecond = -1;
}

void USettingsWidget::ApplyGraphicsPreview()
{
    if (UGameUserSettings* Settings = GetSettings())
    {
        // 화면에만 적용하고 파일 저장은 "유지"를 눌렀을 때 한다
        Settings->ApplyResolutionSettings(false);
        Settings->ApplyNonResolutionSettings();
    }

    PendingRemain = ConfirmSeconds;
    bGraphicsRefreshQueued = true;   // 드롭다운 목록 갱신은 다음 틱에
    UpdateConfirmBanner();
}

void USettingsWidget::KeepGraphics()
{
    if (!bPendingConfirm)
    {
        return;
    }

    bPendingConfirm = false;
    if (UGameUserSettings* Settings = GetSettings())
    {
        Settings->SaveSettings();
    }
    UpdateConfirmBanner();
}

void USettingsWidget::RevertGraphics()
{
    if (!bPendingConfirm)
    {
        return;
    }

    bPendingConfirm = false;
    RestoreGraphics(PendingSnapshot);
    RefreshGraphics();
    UpdateConfirmBanner();
}

void USettingsWidget::UpdateConfirmBanner()
{
    if (!ConfirmBanner || !ConfirmText)
    {
        return;
    }

    // 자리는 늘 차지하고(Hidden) 보일 때만 그린다
    ConfirmBanner->SetVisibility(bPendingConfirm ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (!bPendingConfirm)
    {
        LastShownSecond = -1;
        return;
    }

    const int32 Second = FMath::Max(0, FMath::CeilToInt(PendingRemain));
    if (Second == LastShownSecond)
    {
        return;
    }
    LastShownSecond = Second;

    ConfirmText->SetText(FText::FromString(FString::Printf(
        TEXT("%d초 후 원래대로 돌아갑니다"), Second)));
}

void USettingsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bGraphicsRefreshQueued)
    {
        bGraphicsRefreshQueued = false;
        RefreshGraphics();
    }

    if (!bPendingConfirm)
    {
        return;
    }

    // 해상도가 바뀌는 프레임은 길게 걸릴 수 있어 한 번에 너무 많이 줄지 않게 자른다
    PendingRemain -= FMath::Min(InDeltaTime, 0.25f);
    if (PendingRemain <= 0.f)
    {
        RevertGraphics();
        return;
    }

    UpdateConfirmBanner();
}

void USettingsWidget::OnKeepClicked() { KeepGraphics(); }
void USettingsWidget::OnRevertClicked() { RevertGraphics(); }

void USettingsWidget::HandleClosing()
{
    // 유지를 누르지 않고 닫으면 확인하지 않은 것으로 보고 되돌린다
    RevertGraphics();
}

void USettingsWidget::OnCloseClicked()
{
    // 저장과 되돌리기는 AMyPlayerController::CloseSettingsMenu에서 한다(ESC로 닫을 때도 같은 경로)
    if (OwnerPC)
    {
        OwnerPC->CloseSettingsMenu();
    }
}

FReply USettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // 창에 포커스가 있으면 게임 쪽 키 입력이 안 들어오므로 여기서 닫는다
    if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::F10)
    {
        OnCloseClicked();
        return FReply::Handled();
    }

    // Q / E 로 탭 넘기기
    if (InKeyEvent.GetKey() == EKeys::Q || InKeyEvent.GetKey() == EKeys::E)
    {
        SelectTab(ActiveTab == 0 ? 1 : 0);
        return FReply::Handled();
    }

    // 설정 창이 떠 있는 동안 다른 키는 게임으로 넘기지 않는다
    return FReply::Handled();
}

FReply USettingsWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 창 바깥(어두운 배경)을 눌러도 게임으로 클릭이 새지 않게 막는다
    return FReply::Handled();
}

FReply USettingsWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    return FReply::Handled();
}
