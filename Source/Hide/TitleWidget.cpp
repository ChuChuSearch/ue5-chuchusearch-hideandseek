#include "TitleWidget.h"

#include "NicknameLocalSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"

namespace
{
    const TCHAR* BoldFontPath = TEXT("/Game/Game/Fonts/Maplestory_Bold_Font.Maplestory_Bold_Font");
    const TCHAR* LightFontPath = TEXT("/Game/Game/Fonts/Maplestory_Light_Font.Maplestory_Light_Font");
    const TCHAR* ClickSoundPath = TEXT("/Game/Game/Sounds/UI/SFX_UI_Click.SFX_UI_Click");
    const TCHAR* BackgroundPath = TEXT("/Game/Game/Textures/MainScreenUI/Mainscreen_Background.Mainscreen_Background");
    const TCHAR* LogoPath = TEXT("/Game/Game/Textures/MainScreenUI/Title.Title");
    const TCHAR* WaitRoomMapPath = TEXT("/Game/Game/Levels/WaitRoomMap");

    const TCHAR* ConfigSection = TEXT("Hide.Title");
    const TCHAR* NicknameKey = TEXT("Nickname");
    const TCHAR* AddressKey = TEXT("LastAddress");
    constexpr int32 MaxNicknameLen = 16; // 서버(ServerSetNickname)와 같은 길이

    FLinearColor Hex(uint8 R, uint8 G, uint8 B, uint8 A = 255)
    {
        return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
    }

    // 대기실 패널 / 설정 창과 같은 색
    const FLinearColor PanelOuterFill = Hex(255, 236, 196);
    const FLinearColor PanelOuterLine = Hex(236, 152, 42);
    const FLinearColor PanelInnerFill = Hex(255, 252, 242);
    const FLinearColor PanelInnerLine = Hex(240, 196, 128);
    const FLinearColor RibbonFill = Hex(146, 92, 232);
    const FLinearColor RibbonLine = Hex(92, 46, 170);
    const FLinearColor LabelColor = Hex(122, 74, 30);
    const FLinearColor SubLabelColor = Hex(122, 74, 30, 150);
    const FLinearColor ErrorColor = Hex(214, 64, 70);
    const FLinearColor TrackFill = Hex(244, 228, 198);
    const FLinearColor TrackLine = Hex(218, 180, 120);
    const FLinearColor HostFill = Hex(255, 128, 128);
    const FLinearColor HostHover = Hex(255, 158, 150);
    const FLinearColor HostLine = Hex(214, 82, 90);
    const FLinearColor JoinFill = Hex(96, 156, 236);
    const FLinearColor JoinHover = Hex(128, 178, 246);
    const FLinearColor JoinLine = Hex(52, 104, 190);

    constexpr float CardWidth = 520.f;

    FSlateBrush RoundBox(const FLinearColor& Fill, float Radius, const FLinearColor& Line = FLinearColor::Transparent, float LineWidth = 0.f)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }
}

TSharedRef<SWidget> UTitleWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }
    return Super::RebuildWidget();
}

FSlateFontInfo UTitleWidget::MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor, int32 OutlineSize) const
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

UTextBlock* UTitleWidget::MakeLabel(const FString& Text)
{
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Text));
    Label->SetFont(MakeFont(true, 17));
    Label->SetColorAndOpacity(FSlateColor(LabelColor));
    return Label;
}

UEditableTextBox* UTitleWidget::MakeField(const FName Name, const FString& Hint)
{
    UEditableTextBox* Box = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), Name);
    FEditableTextBoxStyle Style = Box->WidgetStyle;
    Style.SetBackgroundImageNormal(RoundBox(TrackFill, 14.f, TrackLine, 2.f));
    Style.SetBackgroundImageHovered(RoundBox(TrackFill, 14.f, PanelOuterLine, 2.f));
    Style.SetBackgroundImageFocused(RoundBox(Hex(255, 255, 255), 14.f, PanelOuterLine, 2.f));
    Style.SetBackgroundImageReadOnly(RoundBox(TrackFill, 14.f, TrackLine, 2.f));
    Style.SetPadding(FMargin(16.f, 11.f));
    Style.TextStyle.SetFont(MakeFont(false, 19));
    Style.SetForegroundColor(FSlateColor(LabelColor));
    Style.SetFocusedForegroundColor(FSlateColor(LabelColor));
    Style.SetBackgroundColor(FSlateColor(FLinearColor::White));
    Box->WidgetStyle = Style;
    Box->SetHintText(FText::FromString(Hint));
    return Box;
}

UButton* UTitleWidget::MakeButton(const FName Name, const FString& Label, const FLinearColor& Fill, const FLinearColor& Hover, const FLinearColor& Line)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
    FButtonStyle Style = Button->GetStyle();
    Style.SetNormal(RoundBox(Fill, 20.f, Line, 3.f));
    Style.SetHovered(RoundBox(Hover, 20.f, Line, 3.f));
    Style.SetPressed(RoundBox(Line, 20.f, Line, 3.f));
    Style.SetDisabled(RoundBox(Hex(210, 200, 190), 20.f, Hex(170, 160, 150), 3.f));
    Style.SetNormalPadding(FMargin(10.f, 10.f, 10.f, 12.f));
    Style.SetPressedPadding(FMargin(10.f, 11.f, 10.f, 11.f));
    if (ClickSound)
    {
        FSlateSound Click;
        Click.SetResourceObject(ClickSound);
        Style.SetPressedSound(Click);
    }
    Button->SetStyle(Style);

    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(FText::FromString(Label));
    Text->SetFont(MakeFont(true, 22, Line, 2));
    Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Text->SetJustification(ETextJustify::Center);
    Button->SetContent(Text);
    return Button;
}

void UTitleWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    LightFont = LoadObject<UObject>(nullptr, LightFontPath);
    ClickSound = LoadObject<USoundBase>(nullptr, ClickSoundPath);

    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
    WidgetTree->RootWidget = Root;

    // ----- 배경(화면을 꽉 채우고 넘치는 부분은 잘린다) -----
    if (UTexture2D* BgTex = LoadObject<UTexture2D>(nullptr, BackgroundPath))
    {
        UScaleBox* BgScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("BackgroundScale"));
        BgScale->SetStretch(EStretch::ScaleToFill);
        UImage* Bg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Background"));
        Bg->SetBrushFromTexture(BgTex, true);
        BgScale->SetContent(Bg);
        if (UCanvasPanelSlot* S = Root->AddChildToCanvas(BgScale))
        {
            S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            S->SetOffsets(FMargin(0.f));
        }
    }

    // ----- 왼쪽 가운데 세로 한 줄: 로고 + 접속 카드 -----
    USizeBox* ColumnSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ColumnSize"));
    ColumnSize->SetWidthOverride(CardWidth);
    if (UCanvasPanelSlot* S = Root->AddChildToCanvas(ColumnSize))
    {
        S->SetAnchors(FAnchors(0.34f, 0.5f));
        S->SetAlignment(FVector2D(0.5, 0.5));
        S->SetAutoSize(true);
    }
    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
    ColumnSize->SetContent(Column);

    if (UTexture2D* LogoTex = LoadObject<UTexture2D>(nullptr, LogoPath))
    {
        USizeBox* LogoSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LogoSize"));
        LogoSize->SetWidthOverride(340.f);
        LogoSize->SetHeightOverride(340.f * LogoTex->GetSizeY() / FMath::Max(1, LogoTex->GetSizeX()));
        UImage* Logo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Logo"));
        Logo->SetBrushFromTexture(LogoTex, false);
        LogoSize->SetContent(Logo);
        if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(LogoSize))
        {
            S->SetHorizontalAlignment(HAlign_Center);
            S->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
        }
    }

    // 카드: 바깥 테두리 + 안쪽 + 위쪽 리본(대기실 패널과 같은 모양)
    UOverlay* Card = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Card"));
    Column->AddChildToVerticalBox(Card);

    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Outer->SetBrush(RoundBox(PanelOuterFill, 26.f, PanelOuterLine, 4.f));
    Outer->SetPadding(FMargin(7.f));
    if (UOverlaySlot* S = Card->AddChildToOverlay(Outer))
    {
        S->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
    }
    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Inner->SetBrush(RoundBox(PanelInnerFill, 20.f, PanelInnerLine, 2.f));
    Inner->SetPadding(FMargin(26.f, 36.f, 26.f, 20.f));
    Outer->SetContent(Inner);

    UBorder* Ribbon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Ribbon->SetBrush(RoundBox(RibbonFill, 14.f, RibbonLine, 3.f));
    Ribbon->SetPadding(FMargin(34.f, 5.f, 34.f, 7.f));
    if (UOverlaySlot* S = Card->AddChildToOverlay(Ribbon))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Top);
    }
    UTextBlock* RibbonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    RibbonText->SetText(FText::FromString(TEXT("게임 접속")));
    RibbonText->SetFont(MakeFont(true, 22, RibbonLine, 2));
    RibbonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Ribbon->SetContent(RibbonText);

    UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Body"));
    Inner->SetContent(Body);

    auto AddRow = [Body](UWidget* W, float Top)
    {
        if (UVerticalBoxSlot* S = Body->AddChildToVerticalBox(W))
        {
            S->SetHorizontalAlignment(HAlign_Fill);
            S->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
        }
    };

    AddRow(MakeLabel(TEXT("닉네임")), 0.f);
    NicknameBox = MakeField(TEXT("NicknameBox"), TEXT("닉네임을 입력하세요"));
    NicknameBox->OnTextCommitted.AddDynamic(this, &UTitleWidget::OnNicknameCommitted);
    AddRow(NicknameBox, 6.f);

    AddRow(MakeLabel(TEXT("방 주소 (IP)")), 16.f);
    AddressBox = MakeField(TEXT("AddressBox"), TEXT("예: 192.168.0.10  (비우면 이 컴퓨터)"));
    AddressBox->OnTextCommitted.AddDynamic(this, &UTitleWidget::OnAddressCommitted);
    AddRow(AddressBox, 6.f);

    UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
    AddRow(Buttons, 22.f);

    HostButton = MakeButton(TEXT("HostButton"), TEXT("방 만들기"), HostFill, HostHover, HostLine);
    HostButton->OnClicked.AddDynamic(this, &UTitleWidget::OnHostClicked);
    if (UHorizontalBoxSlot* S = Buttons->AddChildToHorizontalBox(HostButton))
    {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));
    }
    JoinButton = MakeButton(TEXT("JoinButton"), TEXT("접속하기"), JoinFill, JoinHover, JoinLine);
    JoinButton->OnClicked.AddDynamic(this, &UTitleWidget::OnJoinClicked);
    if (UHorizontalBoxSlot* S = Buttons->AddChildToHorizontalBox(JoinButton))
    {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetPadding(FMargin(7.f, 0.f, 0.f, 0.f));
    }

    StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
    StatusText->SetFont(MakeFont(false, 14));
    StatusText->SetAutoWrapText(true);
    StatusText->SetJustification(ETextJustify::Center);
    AddRow(StatusText, 14.f);
    SetStatus(TEXT("방장은 \"방 만들기\", 친구는 방장의 IP를 넣고 \"접속하기\""), false);

    // 지난번에 쓴 닉네임 / 주소 채우기(같은 실행 중에는 로컬 플레이어에 기억된 닉네임이 우선)
    FString SavedNick;
    FString SavedAddress;
    if (GConfig)
    {
        GConfig->GetString(ConfigSection, NicknameKey, SavedNick, GGameUserSettingsIni);
        GConfig->GetString(ConfigSection, AddressKey, SavedAddress, GGameUserSettingsIni);
    }
    if (const ULocalPlayer* LP = GetOwningLocalPlayer())
    {
        if (const UNicknameLocalSubsystem* NickSub = LP->GetSubsystem<UNicknameLocalSubsystem>())
        {
            if (!NickSub->GetNickname().IsEmpty())
            {
                SavedNick = NickSub->GetNickname();
            }
        }
    }
    NicknameBox->SetText(FText::FromString(SavedNick.Left(MaxNicknameLen)));
    // 처음 실행이라 지난 주소가 없으면 전용 서버 주소를 채워 둔다(DefaultGame.ini의 [Hide.Title] DefaultServerAddress)
    if (SavedAddress.IsEmpty() && GConfig)
    {
        GConfig->GetString(TEXT("Hide.Title"), TEXT("DefaultServerAddress"), SavedAddress, GGameIni);
    }
    AddressBox->SetText(FText::FromString(SavedAddress));
}

UWidget* UTitleWidget::GetInitialFocusWidget() const
{
    if (NicknameBox && NicknameBox->GetText().IsEmpty())
    {
        return NicknameBox;
    }
    return AddressBox ? static_cast<UWidget*>(AddressBox) : static_cast<UWidget*>(NicknameBox);
}

void UTitleWidget::SetStatus(const FString& Text, bool bError)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(Text));
        StatusText->SetColorAndOpacity(FSlateColor(bError ? ErrorColor : SubLabelColor));
    }
}

bool UTitleWidget::CommitNickname()
{
    FString Nick;
    for (const TCHAR Ch : (NicknameBox ? NicknameBox->GetText().ToString() : FString()))
    {
        if (!FChar::IsControl(Ch))
        {
            Nick.AppendChar(Ch);
        }
    }
    Nick.TrimStartAndEndInline();

    if (Nick.IsEmpty())
    {
        SetStatus(TEXT("닉네임을 먼저 입력해 주세요"), true);
        if (NicknameBox)
        {
            NicknameBox->SetKeyboardFocus();
        }
        return false;
    }
    if (Nick.Len() > MaxNicknameLen)
    {
        SetStatus(FString::Printf(TEXT("닉네임은 %d글자까지 쓸 수 있어요"), MaxNicknameLen), true);
        return false;
    }

    if (const ULocalPlayer* LP = GetOwningLocalPlayer())
    {
        if (UNicknameLocalSubsystem* NickSub = LP->GetSubsystem<UNicknameLocalSubsystem>())
        {
            NickSub->SetNickname(Nick);
        }
    }
    if (GConfig)
    {
        GConfig->SetString(ConfigSection, NicknameKey, *Nick, GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }
    return true;
}

void UTitleWidget::OnHostClicked()
{
    if (bLeaving || !CommitNickname())
    {
        return;
    }

    bLeaving = true;
    SetStatus(TEXT("방을 만드는 중…"), false);
    if (HostButton) HostButton->SetIsEnabled(false);
    if (JoinButton) JoinButton->SetIsEnabled(false);

    // 이 컴퓨터가 서버가 되어 대기실을 연다(다른 사람은 이 컴퓨터 IP로 접속)
    UGameplayStatics::OpenLevel(this, FName(WaitRoomMapPath), true, TEXT("listen"));
}

void UTitleWidget::OnJoinClicked()
{
    if (bLeaving || !CommitNickname())
    {
        return;
    }

    FString Address = AddressBox ? AddressBox->GetText().ToString().TrimStartAndEnd() : FString();
    Address.ReplaceInline(TEXT(" "), TEXT(""));
    if (Address.IsEmpty())
    {
        Address = TEXT("127.0.0.1");
    }

    if (GConfig)
    {
        GConfig->SetString(ConfigSection, AddressKey, *Address, GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }

    bLeaving = true;
    SetStatus(FString::Printf(TEXT("%s 에 접속하는 중…"), *Address), false);
    if (HostButton) HostButton->SetIsEnabled(false);
    if (JoinButton) JoinButton->SetIsEnabled(false);

    // 접속에 실패하면 엔진이 기본 맵(이 화면)으로 돌려보낸다
    PC->ClientTravel(Address, TRAVEL_Absolute);
}

void UTitleWidget::OnNicknameCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    // 닉네임에서 Enter → 주소 칸으로
    if (CommitMethod == ETextCommit::OnEnter && AddressBox)
    {
        AddressBox->SetKeyboardFocus();
    }
}

void UTitleWidget::OnAddressCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    // 주소에서 Enter → 접속하기
    if (CommitMethod == ETextCommit::OnEnter)
    {
        OnJoinClicked();
    }
}
