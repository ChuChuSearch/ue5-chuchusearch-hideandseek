#include "LobbyPanelWidget.h"

#include "MyPlayerController.h"
#include "MyPlayerState.h"
#include "WaitRoomGameMode.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
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

    FLinearColor Hex(uint8 R, uint8 G, uint8 B, uint8 A = 255)
    {
        return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
    }

    // 설정 창 / 점수판과 같은 색
    const FLinearColor PanelOuterFill = Hex(255, 236, 196);
    const FLinearColor PanelOuterLine = Hex(236, 152, 42);
    const FLinearColor PanelInnerFill = Hex(255, 252, 242);
    const FLinearColor PanelInnerLine = Hex(240, 196, 128);
    const FLinearColor RibbonFill = Hex(146, 92, 232);
    const FLinearColor RibbonLine = Hex(92, 46, 170);
    const FLinearColor LabelColor = Hex(122, 74, 30);
    const FLinearColor SubLabelColor = Hex(122, 74, 30, 150);
    const FLinearColor AccentColor = Hex(255, 128, 128);
    const FLinearColor AccentHover = Hex(255, 158, 150);
    const FLinearColor AccentDark = Hex(214, 82, 90);
    const FLinearColor SeekerColor = Hex(214, 64, 70);
    const FLinearColor RunnerColor = Hex(56, 150, 96);
    const FLinearColor AnyColor = Hex(160, 140, 120);
    const FLinearColor ReadyColor = Hex(64, 170, 220);
    const FLinearColor HostFill = Hex(255, 214, 96);
    const FLinearColor HostText = Hex(150, 96, 10);
    const FLinearColor RowFill = Hex(250, 240, 220);
    const FLinearColor RowMineFill = Hex(255, 220, 206);
    const FLinearColor RowMineLine = Hex(255, 128, 128);
    const FLinearColor TrackFill = Hex(244, 228, 198);
    const FLinearColor TrackLine = Hex(218, 180, 120);

    constexpr float PanelWidth = 440.f;
    constexpr float ChatHeight = 170.f;
    constexpr float ChatWidth = 400.f;
    constexpr int32 MaxChatLines = 60;
    constexpr float RefreshInterval = 0.25f;

    FSlateBrush RoundBox(const FLinearColor& Fill, float Radius, const FLinearColor& Line = FLinearColor::Transparent, float LineWidth = 0.f)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }
}

TSharedRef<SWidget> ULobbyPanelWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

FSlateFontInfo ULobbyPanelWidget::MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor, int32 OutlineSize) const
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

UWidget* ULobbyPanelWidget::MakePanel(const FString& Title, UVerticalBox*& OutBody, UTextBlock** OutTitleText)
{
    UOverlay* Card = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Outer->SetBrush(RoundBox(PanelOuterFill, 24.f, PanelOuterLine, 4.f));
    Outer->SetPadding(FMargin(7.f));
    if (UOverlaySlot* S = Card->AddChildToOverlay(Outer))
    {
        S->SetPadding(FMargin(0.f, 20.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
    }

    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Inner->SetBrush(RoundBox(PanelInnerFill, 18.f, PanelInnerLine, 2.f));
    Inner->SetPadding(FMargin(18.f, 32.f, 18.f, 16.f));
    Outer->SetContent(Inner);

    OutBody = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Inner->SetContent(OutBody);

    UBorder* Ribbon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Ribbon->SetBrush(RoundBox(RibbonFill, 13.f, RibbonLine, 3.f));
    Ribbon->SetPadding(FMargin(30.f, 5.f, 30.f, 7.f));
    if (UOverlaySlot* S = Card->AddChildToOverlay(Ribbon))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Top);
    }

    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    TitleText->SetText(FText::FromString(Title));
    TitleText->SetFont(MakeFont(true, 20, RibbonLine, 2));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Ribbon->SetContent(TitleText);

    if (OutTitleText)
    {
        *OutTitleText = TitleText;
    }

    return Card;
}

UWidget* ULobbyPanelWidget::MakeChip(const FString& Text, const FLinearColor& Fill, const FLinearColor& TextColor, int32 FontSize)
{
    UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Chip->SetBrush(RoundBox(Fill, 9.f));
    Chip->SetPadding(FMargin(9.f, 2.f, 9.f, 4.f));
    UTextBlock* ChipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    ChipText->SetText(FText::FromString(Text));
    ChipText->SetFont(MakeFont(true, FontSize));
    ChipText->SetColorAndOpacity(FSlateColor(TextColor));
    Chip->SetContent(ChipText);
    return Chip;
}

void ULobbyPanelWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    LightFont = LoadObject<UObject>(nullptr, LightFontPath);
    ClickSound = LoadObject<USoundBase>(nullptr, ClickSoundPath);

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
    WidgetTree->RootWidget = Root;

    // 로비 맵에서는 캐릭터로 돌아다니므로 화면을 가리지 않게:
    // - 참가자 목록은 Tab을 누르고 있는 동안만 화면 가운데에
    // - 채팅은 왼쪽 아래에 작게. Enter로 입력 칸을 열고, Enter로 보내면 닫힌다(Esc는 취소)

    // ----- 참가자 (Tab) -----
    USizeBox* PlayersSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PlayersSize"));
    PlayersSize->SetWidthOverride(PanelWidth);
    if (UOverlaySlot* S = Root->AddChildToOverlay(PlayersSize))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Center);
    }
    PlayersPanelRoot = PlayersSize;

    UVerticalBox* PlayersBody = nullptr;
    UTextBlock* PlayersTitle = nullptr;
    UWidget* PlayersPanel = MakePanel(TEXT("참가자"), PlayersBody, &PlayersTitle);
    PlayerTitle = PlayersTitle;
    PlayersSize->SetContent(PlayersPanel);

    PlayerRows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PlayerRows"));
    PlayersBody->AddChildToVerticalBox(PlayerRows);

    PlayersHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayersHint"));
    PlayersHint->SetFont(MakeFont(false, 12));
    PlayersHint->SetColorAndOpacity(FSlateColor(SubLabelColor));
    PlayersHint->SetAutoWrapText(true);
    PlayersHint->SetJustification(ETextJustify::Center);
    if (UVerticalBoxSlot* S = PlayersBody->AddChildToVerticalBox(PlayersHint))
    {
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
    }
    PlayersSize->SetVisibility(ESlateVisibility::Collapsed);

    // ----- 채팅 (왼쪽 아래) -----
    USizeBox* ChatColumn = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ChatColumn"));
    ChatColumn->SetWidthOverride(ChatWidth);
    if (UOverlaySlot* S = Root->AddChildToOverlay(ChatColumn))
    {
        S->SetHorizontalAlignment(HAlign_Left);
        S->SetVerticalAlignment(VAlign_Bottom);
        // 왼쪽 아래 "H 조작법" 칩 위
        S->SetPadding(FMargin(24.f, 0.f, 0.f, 84.f));
        ChatColumnSlot = S;
    }

    ChatPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChatPanel"));
    ChatPanel->SetPadding(FMargin(12.f, 10.f, 12.f, 10.f));
    ChatColumn->SetContent(ChatPanel);

    UVerticalBox* ChatBody = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChatBody"));
    ChatPanel->SetContent(ChatBody);

    USizeBox* ChatSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ChatSize"));
    ChatSize->SetHeightOverride(ChatHeight);
    ChatBody->AddChildToVerticalBox(ChatSize);

    ChatScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ChatScroll"));
    ChatScroll->SetScrollbarThickness(FVector2D(5.0, 5.0));
    ChatSize->SetContent(ChatScroll);

    // 닫혀 있을 때 안내 한 줄
    ChatHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ChatHint"));
    ChatHint->SetText(FText::FromString(TEXT("Enter  채팅   ·   Tab  참가자")));
    ChatHint->SetFont(MakeFont(false, 12));
    ChatHint->SetColorAndOpacity(FSlateColor(SubLabelColor));
    if (UVerticalBoxSlot* S = ChatBody->AddChildToVerticalBox(ChatHint))
    {
        S->SetPadding(FMargin(2.f, 6.f, 0.f, 0.f));
    }

    // 입력 줄: 글 상자 + 보내기 버튼(Enter를 누르면 나타난다)
    UHorizontalBox* InputLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InputLine"));
    if (UVerticalBoxSlot* S = ChatBody->AddChildToVerticalBox(InputLine))
    {
        S->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
    }
    ChatInputLine = InputLine;

    ChatInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("ChatInput"));
    {
        FEditableTextBoxStyle Style = ChatInput->WidgetStyle;
        Style.SetBackgroundImageNormal(RoundBox(TrackFill, 12.f, TrackLine, 2.f));
        Style.SetBackgroundImageHovered(RoundBox(TrackFill, 12.f, PanelOuterLine, 2.f));
        Style.SetBackgroundImageFocused(RoundBox(Hex(255, 255, 255), 12.f, PanelOuterLine, 2.f));
        Style.SetBackgroundImageReadOnly(RoundBox(TrackFill, 12.f, TrackLine, 2.f));
        Style.SetPadding(FMargin(12.f, 7.f));
        Style.TextStyle.SetFont(MakeFont(false, 15));
        Style.SetForegroundColor(FSlateColor(LabelColor));
        Style.SetFocusedForegroundColor(FSlateColor(LabelColor));
        Style.SetBackgroundColor(FSlateColor(FLinearColor::White));
        ChatInput->WidgetStyle = Style;
    }
    ChatInput->SetHintText(FText::FromString(TEXT("Enter 보내기 · Esc 닫기")));
    ChatInput->SetClearKeyboardFocusOnCommit(false);
    ChatInput->SetSelectAllTextOnCommit(false);
    ChatInput->SetRevertTextOnEscape(false);
    ChatInput->OnTextCommitted.AddDynamic(this, &ULobbyPanelWidget::OnChatCommitted);
    ChatInput->OnTextChanged.AddDynamic(this, &ULobbyPanelWidget::OnChatChanged);
    if (UHorizontalBoxSlot* S = InputLine->AddChildToHorizontalBox(ChatInput))
    {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetVerticalAlignment(VAlign_Center);
    }

    SendButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SendButton"));
    {
        FButtonStyle Style = SendButton->GetStyle();
        Style.SetNormal(RoundBox(AccentColor, 12.f, AccentDark, 2.f));
        Style.SetHovered(RoundBox(AccentHover, 12.f, AccentDark, 2.f));
        Style.SetPressed(RoundBox(AccentDark, 12.f, AccentDark, 2.f));
        Style.SetNormalPadding(FMargin(12.f, 5.f));
        Style.SetPressedPadding(FMargin(12.f, 6.f, 12.f, 4.f));
        if (ClickSound)
        {
            FSlateSound Click;
            Click.SetResourceObject(ClickSound);
            Style.SetPressedSound(Click);
        }
        SendButton->SetStyle(Style);
    }
    UTextBlock* SendText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SendText"));
    SendText->SetText(FText::FromString(TEXT("보내기")));
    SendText->SetFont(MakeFont(true, 14, AccentDark, 1));
    SendText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    SendButton->SetContent(SendText);
    SendButton->OnClicked.AddDynamic(this, &ULobbyPanelWidget::OnSendClicked);
    if (UHorizontalBoxSlot* S = InputLine->AddChildToHorizontalBox(SendButton))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
    }

    ApplyChatOpenLook(false);

    AddChatLine(FString(), TEXT("대기실 채팅이에요. Enter를 눌러 인사해 보세요!"), true);

    RefreshPlayers();
}

void ULobbyPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    RefreshRemain -= InDeltaTime;
    if (RefreshRemain <= 0.f)
    {
        RefreshRemain = RefreshInterval;
        RefreshPlayers();
    }

    if (bScrollToEndPending && ChatScroll)
    {
        bScrollToEndPending = false;
        ChatScroll->ScrollToEnd();
    }
}

// ===== 참가자 목록 =====

void ULobbyPanelWidget::RefreshPlayers()
{
    if (!PlayerRows)
    {
        return;
    }

    const UWorld* World = GetWorld();
    const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
    const APlayerController* PC = GetOwningPlayer();
    const APlayerState* LocalPS = PC ? PC->PlayerState.Get() : nullptr;

    TArray<const AMyPlayerState*> Players;
    if (GS)
    {
        for (const APlayerState* PS : GS->PlayerArray)
        {
            if (const AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS))
            {
                Players.Add(MyPS);
            }
        }
    }

    // 들어온 순서대로(방장이 보통 맨 위)
    Players.Sort([](const AMyPlayerState& A, const AMyPlayerState& B)
    {
        const int32 AOrder = A.GetLobbyJoinOrder() > 0 ? A.GetLobbyJoinOrder() : MAX_int32;
        const int32 BOrder = B.GetLobbyJoinOrder() > 0 ? B.GetLobbyJoinOrder() : MAX_int32;
        return AOrder < BOrder;
    });

    // 보이는 내용이 그대로면 다시 그리지 않는다
    FString Signature;
    for (const AMyPlayerState* PS : Players)
    {
        Signature += FString::Printf(TEXT("%p|%s|%d|%d|%d|%d;"), PS, *PS->GetNickname(),
            static_cast<int32>(PS->GetPreference()), PS->IsHost() ? 1 : 0, PS == LocalPS ? 1 : 0, PS->IsReady() ? 1 : 0);
    }
    const AMyPlayerController* MyPC = Cast<AMyPlayerController>(PC);
    const int32 MaxPlayers = MyPC ? MyPC->GetLobbyMaxPlayers() : 0;
    Signature += FString::Printf(TEXT("max=%d"), MaxPlayers);

    if (Signature == LastPlayersSignature)
    {
        return;
    }
    LastPlayersSignature = Signature;

    if (PlayerTitle)
    {
        // 최대 인원을 알면 "참가자 2/4", 아직 모르면 "참가자 2명"
        PlayerTitle->SetText(FText::FromString(MaxPlayers > 0
            ? FString::Printf(TEXT("참가자 %d/%d"), Players.Num(), MaxPlayers)
            : FString::Printf(TEXT("참가자 %d명"), Players.Num())));
    }

    PlayerRows->ClearChildren();
    for (const AMyPlayerState* PS : Players)
    {
        AddPlayerRow(PS, PS == LocalPS);
    }

    UpdateReadyArea(Players, Cast<AMyPlayerState>(LocalPS));

    // 남은 자리는 점선 느낌의 빈 줄로 보여 준다
    for (int32 i = Players.Num(); i < MaxPlayers; ++i)
    {
        UBorder* EmptyRow = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        EmptyRow->SetBrush(RoundBox(FLinearColor::Transparent, 12.f, TrackLine, 1.5f));
        EmptyRow->SetPadding(FMargin(10.f, 6.f, 10.f, 8.f));
        UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        EmptyText->SetText(FText::FromString(TEXT("빈 자리")));
        EmptyText->SetFont(MakeFont(false, 15));
        EmptyText->SetColorAndOpacity(FSlateColor(SubLabelColor));
        EmptyRow->SetContent(EmptyText);
        if (UVerticalBoxSlot* S = PlayerRows->AddChildToVerticalBox(EmptyRow))
        {
            S->SetPadding(FMargin(0.f, 3.f));
        }
    }

    if (Players.Num() == 0)
    {
        UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Empty->SetText(FText::FromString(TEXT("접속 중…")));
        Empty->SetFont(MakeFont(false, 14));
        Empty->SetColorAndOpacity(FSlateColor(SubLabelColor));
        PlayerRows->AddChildToVerticalBox(Empty);
    }
}

void ULobbyPanelWidget::AddPlayerRow(const AMyPlayerState* PS, bool bLocal)
{
    UBorder* RowBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    RowBg->SetBrush(bLocal ? RoundBox(RowMineFill, 12.f, RowMineLine, 2.f) : RoundBox(RowFill, 12.f));
    RowBg->SetPadding(FMargin(10.f, 6.f, 10.f, 8.f));

    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    RowBg->SetContent(Line);

    // 닉네임(길면 …으로 줄임). 아직 안 정했으면 흐리게 안내.
    const FString Nickname = PS->GetNickname();
    UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    NameText->SetText(FText::FromString(Nickname.IsEmpty() ? TEXT("닉네임 입력 중…") : Nickname));
    NameText->SetFont(MakeFont(!Nickname.IsEmpty(), 17));
    NameText->SetColorAndOpacity(FSlateColor(Nickname.IsEmpty() ? SubLabelColor : (bLocal ? AccentDark : LabelColor)));
    NameText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
    NameText->SetClipping(EWidgetClipping::ClipToBounds);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(NameText))
    {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (bLocal)
    {
        if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(MakeChip(TEXT("나"), AccentDark, FLinearColor::White, 12)))
        {
            S->SetVerticalAlignment(VAlign_Center);
            S->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
        }
    }

    // 역할 지망
    FString PrefText = TEXT("상관없음");
    FLinearColor PrefColor = AnyColor;
    switch (PS->GetPreference())
    {
    case ERolePreference::SeekerWish:
        PrefText = TEXT("술래 지망");
        PrefColor = SeekerColor;
        break;
    case ERolePreference::RunnerWish:
        PrefText = TEXT("러너 지망");
        PrefColor = RunnerColor;
        break;
    default:
        break;
    }

    USizeBox* PrefBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    PrefBox->SetMinDesiredWidth(92.f);
    PrefBox->SetContent(MakeChip(PrefText, PrefColor, FLinearColor::White, 13));
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(PrefBox))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
    }

    // 상태: 방장 / 준비 완료 / 준비 중
    FString StatusText = TEXT("준비 중");
    FLinearColor StatusFill = Hex(230, 220, 204);
    FLinearColor StatusTextColor = SubLabelColor;
    if (PS->IsHost())
    {
        StatusText = TEXT("방장");
        StatusFill = HostFill;
        StatusTextColor = HostText;
    }
    else if (PS->IsReady())
    {
        StatusText = TEXT("준비 완료");
        StatusFill = ReadyColor;
        StatusTextColor = FLinearColor::White;
    }

    USizeBox* StatusBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    StatusBox->SetWidthOverride(84.f);
    UWidget* StatusChip = MakeChip(StatusText, StatusFill, StatusTextColor, 13);
    if (UBorder* ChipBorder = Cast<UBorder>(StatusChip))
    {
        ChipBorder->SetHorizontalAlignment(HAlign_Center);
    }
    StatusBox->SetContent(StatusChip);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(StatusBox))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
    }

    if (UVerticalBoxSlot* S = PlayerRows->AddChildToVerticalBox(RowBg))
    {
        S->SetPadding(FMargin(0.f, 3.f));
    }
}

void ULobbyPanelWidget::UpdateReadyArea(const TArray<const AMyPlayerState*>& Players, const AMyPlayerState* LocalPS)
{
    if (!PlayersHint)
    {
        return;
    }

    int32 Guests = 0;
    int32 ReadyCount = 0;
    for (const AMyPlayerState* PS : Players)
    {
        if (!PS->IsHost())
        {
            ++Guests;
            ReadyCount += PS->IsReady() ? 1 : 0;
        }
    }

    const bool bHost = LocalPS && LocalPS->IsHost();

    FString Hint;
    if (Players.Num() < 2)
    {
        Hint = TEXT("2명 이상 모이면 게임을 시작할 수 있어요");
    }
    else if (ReadyCount < Guests)
    {
        Hint = FString::Printf(TEXT("준비 %d/%d · 출발 발판 위에 서면 준비 완료"), ReadyCount, Guests);
    }
    else
    {
        Hint = bHost ? TEXT("모두 준비했어요! 발판 위에서 E를 눌러 출발하세요") : TEXT("모두 준비했어요! 방장이 출발하면 바로 시작해요");
    }
    PlayersHint->SetText(FText::FromString(Hint));
}

// ===== 채팅 =====

void ULobbyPanelWidget::AddChatLine(const FString& SenderName, const FString& Message, bool bSystem)
{
    if (!ChatScroll)
    {
        return;
    }

    // 오래된 줄은 지운다
    while (ChatScroll->GetChildrenCount() >= MaxChatLines)
    {
        ChatScroll->RemoveChildAt(0);
    }

    UWidget* LineWidget = nullptr;

    if (bSystem)
    {
        UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        T->SetText(FText::FromString(Message));
        T->SetFont(MakeFont(false, 13));
        T->SetColorAndOpacity(FSlateColor(SubLabelColor));
        T->SetAutoWrapText(true);
        T->SetJustification(ETextJustify::Center);
        LineWidget = T;
    }
    else
    {
        // 내 닉네임이면 코랄색으로
        const APlayerController* PC = GetOwningPlayer();
        const AMyPlayerState* LocalPS = PC ? PC->GetPlayerState<AMyPlayerState>() : nullptr;
        const bool bMine = LocalPS && !SenderName.IsEmpty() && LocalPS->GetNickname() == SenderName;

        UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

        UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Name->SetText(FText::FromString(SenderName));
        Name->SetFont(MakeFont(true, 15));
        Name->SetColorAndOpacity(FSlateColor(bMine ? AccentDark : RibbonFill));
        if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Name))
        {
            S->SetVerticalAlignment(VAlign_Top);
        }

        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Text->SetText(FText::FromString(Message));
        Text->SetFont(MakeFont(false, 15));
        Text->SetColorAndOpacity(FSlateColor(LabelColor));
        Text->SetAutoWrapText(true);
        if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Text))
        {
            S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            S->SetVerticalAlignment(VAlign_Top);
            S->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
        }

        LineWidget = Line;
    }

    if (UScrollBoxSlot* S = Cast<UScrollBoxSlot>(ChatScroll->AddChild(LineWidget)))
    {
        S->SetPadding(FMargin(2.f, 3.f, 8.f, 3.f));
    }

    // 새 줄이 레이아웃에 잡힌 다음 맨 아래로
    bScrollToEndPending = true;
}

void ULobbyPanelWidget::OnGameSettingsClicked()
{
    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
    {
        PC->ToggleLobbySettingsWindow();
    }
}

void ULobbyPanelWidget::OnSettingsClicked()
{
    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
    {
        PC->ToggleSettingsMenu();
    }
}

void ULobbyPanelWidget::SubmitChat()
{
    if (!ChatInput)
    {
        return;
    }

    const FString Message = ChatInput->GetText().ToString().TrimStartAndEnd();
    if (!Message.IsEmpty())
    {
        if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
        {
            PC->SendLobbyChat(Message);
        }
    }

    ChatInput->SetText(FText::GetEmpty());
}

void ULobbyPanelWidget::OnChatCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (!bChatOpen)
    {
        return;
    }

    // Enter: 보내고 닫는다. Esc / 다른 곳 클릭: 그냥 닫는다(쓰던 글은 남겨 둔다)
    if (CommitMethod == ETextCommit::OnEnter)
    {
        SubmitChat();
    }
    CloseChatInput();
}

void ULobbyPanelWidget::OnChatChanged(const FText& Text)
{
    // 서버 제한보다 길게는 못 치게
    const FString Current = Text.ToString();
    if (ChatInput && Current.Len() > AWaitRoomGameMode::MaxChatLength)
    {
        ChatInput->SetText(FText::FromString(Current.Left(AWaitRoomGameMode::MaxChatLength)));
    }
}

void ULobbyPanelWidget::OnSendClicked()
{
    SubmitChat();
    CloseChatInput();
}

void ULobbyPanelWidget::SetChatRaised(bool bRaised)
{
    if (bChatRaised == bRaised || !ChatColumnSlot)
    {
        return;
    }
    bChatRaised = bRaised;
    // 유령 화면의 물체 버튼(왼쪽 아래)과 겹치지 않게 그 위로 올린다
    ChatColumnSlot->SetPadding(FMargin(24.f, 0.f, 0.f, bRaised ? 300.f : 84.f));
}

void ULobbyPanelWidget::SetPlayersVisible(bool bVisible)
{
    if (PlayersPanelRoot)
    {
        PlayersPanelRoot->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (bVisible)
    {
        LastPlayersSignature.Empty();
        RefreshPlayers();
    }
}

void ULobbyPanelWidget::ApplyChatOpenLook(bool bOpen)
{
    if (ChatPanel)
    {
        // 닫혀 있으면 흐릿하게, 열면 또렷하게
        ChatPanel->SetBrush(bOpen
            ? RoundBox(PanelInnerFill, 16.f, PanelOuterLine, 3.f)
            : RoundBox(Hex(255, 252, 242, 120), 16.f, Hex(240, 196, 128, 120), 2.f));
    }
    if (ChatInputLine)
    {
        ChatInputLine->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    if (ChatHint)
    {
        ChatHint->SetVisibility(bOpen ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }
}

void ULobbyPanelWidget::OpenChatInput()
{
    if (bChatOpen || !ChatInput)
    {
        return;
    }
    bChatOpen = true;
    ApplyChatOpenLook(true);
    ChatInput->SetKeyboardFocus();
}

void ULobbyPanelWidget::CloseChatInput()
{
    if (!bChatOpen)
    {
        return;
    }
    bChatOpen = false;
    ApplyChatOpenLook(false);

    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
    {
        PC->OnLobbyChatClosed();
    }
}
