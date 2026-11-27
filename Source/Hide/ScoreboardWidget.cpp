#include "ScoreboardWidget.h"

#include "MyPlayerState.h"
#include "MyPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"

namespace
{
    const TCHAR* BoldFontPath = TEXT("/Game/Game/Fonts/Maplestory_Bold_Font.Maplestory_Bold_Font");
    const TCHAR* LightFontPath = TEXT("/Game/Game/Fonts/Maplestory_Light_Font.Maplestory_Light_Font");

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
    const FLinearColor AccentDark = Hex(214, 82, 90);

    // 팀 색(조작법 안내 리본과 같은 색)
    const FLinearColor SeekerColor = Hex(214, 64, 70);
    const FLinearColor RunnerColor = Hex(56, 150, 96);
    const FLinearColor GhostColor = Hex(146, 92, 232);
    const FLinearColor CommonColor = Hex(236, 152, 42);

    const FLinearColor RowAltFill = Hex(250, 238, 214, 150);
    const FLinearColor RowMineFill = Hex(255, 214, 200);
    const FLinearColor RowMineLine = Hex(255, 128, 128);

    // 점수판 칸 폭
    constexpr float RankWidth = 44.f;
    constexpr float NameWidth = 230.f;
    constexpr float StatusWidth = 90.f;
    constexpr float RecordWidth = 280.f;
    constexpr float ScoreWidth = 90.f;
    constexpr float HelpWidth = 380.f;

    constexpr float RefreshInterval = 0.25f;

    FSlateBrush RoundBox(const FLinearColor& Fill, float Radius, const FLinearColor& Line = FLinearColor::Transparent, float LineWidth = 0.f)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }
}

TSharedRef<SWidget> UScoreboardWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

FSlateFontInfo UScoreboardWidget::MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor, int32 OutlineSize) const
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

UBorder* UScoreboardWidget::MakePanel(const FString& Title, UVerticalBox*& OutBody, float MinWidth)
{
    // 패널 위에 리본 제목을 겹쳐 올린다(설정 창과 같은 구조)
    UOverlay* Card = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Outer->SetBrush(RoundBox(PanelOuterFill, 26.f, PanelOuterLine, 4.f));
    Outer->SetPadding(FMargin(8.f));
    if (UOverlaySlot* S = Card->AddChildToOverlay(Outer))
    {
        S->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
    }

    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Inner->SetBrush(RoundBox(PanelInnerFill, 20.f, PanelInnerLine, 2.f));
    Inner->SetPadding(FMargin(26.f, 38.f, 26.f, 20.f));
    Outer->SetContent(Inner);

    USizeBox* MinSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    MinSize->SetWidthOverride(MinWidth);
    Inner->SetContent(MinSize);

    OutBody = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    MinSize->SetContent(OutBody);

    UBorder* Ribbon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Ribbon->SetBrush(RoundBox(RibbonFill, 14.f, RibbonLine, 3.f));
    Ribbon->SetPadding(FMargin(36.f, 6.f, 36.f, 8.f));
    if (UOverlaySlot* S = Card->AddChildToOverlay(Ribbon))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Top);
    }

    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    TitleText->SetText(FText::FromString(Title));
    TitleText->SetFont(MakeFont(true, 24, RibbonLine, 2));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Ribbon->SetContent(TitleText);

    // 겉을 감싸는 투명 테두리(배치용)
    UBorder* Wrap = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Wrap->SetBrushColor(FLinearColor::Transparent);
    Wrap->SetPadding(FMargin(0.f));
    Wrap->SetContent(Card);
    return Wrap;
}

void UScoreboardWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    LightFont = LoadObject<UObject>(nullptr, LightFontPath);

    // 게임 화면이 살짝 보이도록 옅게 어둡게
    UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
    Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.35f));
    Dim->SetHorizontalAlignment(HAlign_Center);
    Dim->SetVerticalAlignment(VAlign_Center);
    WidgetTree->RootWidget = Dim;

    UHorizontalBox* Panels = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Panels"));
    Dim->SetContent(Panels);

    UVerticalBox* BoardBody = nullptr;
    UBorder* Board = MakePanel(TEXT("점수판"), BoardBody, RankWidth + NameWidth + StatusWidth + RecordWidth + ScoreWidth + 24.f);
    if (UHorizontalBoxSlot* S = Panels->AddChildToHorizontalBox(Board))
    {
        S->SetVerticalAlignment(VAlign_Top);
    }

    Rows = BoardBody;

    UVerticalBox* HelpPanelBody = nullptr;
    UBorder* Help = MakePanel(TEXT("점수 얻는 법"), HelpPanelBody, HelpWidth);
    if (UHorizontalBoxSlot* S = Panels->AddChildToHorizontalBox(Help))
    {
        S->SetVerticalAlignment(VAlign_Top);
        S->SetPadding(FMargin(20.f, 0.f, 0.f, 0.f));
    }

    HelpBody = HelpPanelBody;

    // 클릭을 막지 않는다
    SetVisibility(ESlateVisibility::HitTestInvisible);

    RefreshNow();
}

void UScoreboardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    RefreshRemain -= InDeltaTime;
    if (RefreshRemain <= 0.f)
    {
        RefreshNow();
    }
}

void UScoreboardWidget::RefreshNow()
{
    RefreshRemain = RefreshInterval;
    RebuildRows();

    const int32 Group = static_cast<int32>(GetLocalHelpGroup());
    if (Group != LastHelpGroup)
    {
        LastHelpGroup = Group;
        RebuildHelp();
    }
}

// ===== 점수판 =====

FString UScoreboardWidget::MakeStatusText(const AMyPlayerState* PS, FLinearColor& OutColor) const
{
    if (PS->GetFinalRole() == EFinalRole::Seeker)
    {
        OutColor = SeekerColor;
        return TEXT("술래");
    }

    if (PS->IsRoundEliminated())
    {
        OutColor = GhostColor;
        return TEXT("유령");
    }

    OutColor = RunnerColor;
    return TEXT("생존");
}

FString UScoreboardWidget::MakeRecordText(const AMyPlayerState* PS, const FHideScoreStats& Stats) const
{
    TArray<FString> Parts;

    if (PS->GetFinalRole() == EFinalRole::Seeker)
    {
        Parts.Add(FString::Printf(TEXT("적중 %d"), Stats.Hits));
        Parts.Add(FString::Printf(TEXT("탈락 %d"), Stats.Eliminations));
    }
    else
    {
        Parts.Add(FString::Printf(TEXT("단서 %d"), Stats.Clues));
        if (Stats.CloseHidePoints > 0)
        {
            Parts.Add(FString::Printf(TEXT("코앞 %d"), Stats.CloseHidePoints));
        }
        if (Stats.NearMisses > 0)
        {
            Parts.Add(FString::Printf(TEXT("회피 %d"), Stats.NearMisses));
        }
        if (Stats.DecoyHits > 0)
        {
            Parts.Add(FString::Printf(TEXT("속임수 %d"), Stats.DecoyHits));
        }
    }

    if (Stats.bPasswordSolved)
    {
        Parts.Add(TEXT("비밀번호!"));
    }

    return FString::Join(Parts, TEXT(" · "));
}

void UScoreboardWidget::AddTeamHeader(const FString& Title, const FLinearColor& Color, int32 Count)
{
    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Chip->SetBrush(RoundBox(Color, 12.f));
    Chip->SetPadding(FMargin(14.f, 3.f, 14.f, 5.f));
    UTextBlock* ChipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    ChipText->SetText(FText::FromString(FString::Printf(TEXT("%s  %d명"), *Title, Count)));
    ChipText->SetFont(MakeFont(true, 16));
    ChipText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Chip->SetContent(ChipText);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Chip))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (UVerticalBoxSlot* S = Rows->AddChildToVerticalBox(Line))
    {
        S->SetPadding(FMargin(0.f, 10.f, 0.f, 4.f));
    }
}

void UScoreboardWidget::AddColumnHeader()
{
    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    auto AddCell = [&](const FString& Text, float Width, ETextJustify::Type Justify)
    {
        USizeBox* Cell = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        Cell->SetWidthOverride(Width);
        UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        T->SetText(FText::FromString(Text));
        T->SetFont(MakeFont(true, 13));
        T->SetColorAndOpacity(FSlateColor(SubLabelColor));
        T->SetJustification(Justify);
        Cell->SetContent(T);
        Line->AddChildToHorizontalBox(Cell);
    };

    AddCell(TEXT("#"), RankWidth, ETextJustify::Center);
    AddCell(TEXT("이름"), NameWidth, ETextJustify::Left);
    AddCell(TEXT("상태"), StatusWidth, ETextJustify::Center);
    AddCell(TEXT("기록"), RecordWidth, ETextJustify::Left);
    AddCell(TEXT("점수"), ScoreWidth, ETextJustify::Right);

    if (UVerticalBoxSlot* S = Rows->AddChildToVerticalBox(Line))
    {
        S->SetPadding(FMargin(12.f, 0.f, 12.f, 2.f));
    }
}

UScoreboardWidget::FRowWidgets UScoreboardWidget::AddPlayerRow(bool bIsLocal, int32 Rank)
{
    FRowWidgets Row;

    UBorder* RowBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    if (bIsLocal)
    {
        RowBg->SetBrush(RoundBox(RowMineFill, 12.f, RowMineLine, 2.f));
    }
    else if (Rank % 2 == 0)
    {
        RowBg->SetBrush(RoundBox(RowAltFill, 12.f));
    }
    else
    {
        RowBg->SetBrushColor(FLinearColor::Transparent);
    }
    RowBg->SetPadding(FMargin(12.f, 5.f, 12.f, 7.f));

    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    RowBg->SetContent(Line);

    auto AddText = [&](UPanelWidget* Parent, const FString& Text, float Width, bool bBold, int32 Size, const FLinearColor& Color, ETextJustify::Type Justify) -> UTextBlock*
    {
        USizeBox* Cell = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        Cell->SetWidthOverride(Width);
        Cell->SetClipping(EWidgetClipping::ClipToBounds);
        UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        T->SetText(FText::FromString(Text));
        T->SetFont(MakeFont(bBold, Size));
        T->SetColorAndOpacity(FSlateColor(Color));
        T->SetJustification(Justify);
        Cell->SetContent(T);
        if (UHorizontalBoxSlot* S = Cast<UHorizontalBox>(Parent)->AddChildToHorizontalBox(Cell))
        {
            S->SetVerticalAlignment(VAlign_Center);
        }
        return T;
    };

    AddText(Line, Rank > 0 ? FString::FromInt(Rank) : FString(TEXT("-")), RankWidth, true, 16, SubLabelColor, ETextJustify::Center);
    Row.Name = AddText(Line, FString(), NameWidth, true, 17, bIsLocal ? AccentDark : LabelColor, ETextJustify::Left);
    Row.Status = AddText(Line, FString(), StatusWidth, true, 15, LabelColor, ETextJustify::Center);
    Row.Record = AddText(Line, FString(), RecordWidth, false, 15, LabelColor, ETextJustify::Left);
    Row.Score = AddText(Line, FString(), ScoreWidth, true, 20, bIsLocal ? AccentDark : LabelColor, ETextJustify::Right);

    if (UVerticalBoxSlot* S = Rows->AddChildToVerticalBox(RowBg))
    {
        S->SetPadding(FMargin(0.f, 2.f));
    }

    return Row;
}

void UScoreboardWidget::FillRow(const FRowWidgets& Row, const AMyPlayerState* PS, bool bIsLocal) const
{
    if (!PS || !Row.Name)
    {
        return;
    }

    FString Name = PS->GetNickname();
    if (Name.IsEmpty())
    {
        Name = PS->GetPlayerName();
    }
    if (PS->IsHost())
    {
        Name += TEXT("  (방장)");
    }
    if (bIsLocal)
    {
        Name += TEXT("  · 나");
    }

    // 글자가 같으면 건드리지 않는다
    auto SetIfChanged = [](UTextBlock* T, const FString& S)
    {
        if (T && !T->GetText().ToString().Equals(S))
        {
            T->SetText(FText::FromString(S));
        }
    };

    SetIfChanged(Row.Name, Name);

    FLinearColor StatusColor = LabelColor;
    SetIfChanged(Row.Status, MakeStatusText(PS, StatusColor));
    Row.Status->SetColorAndOpacity(FSlateColor(StatusColor));

    FHideScoreStats Stats;
    if (GetVisibleStats(PS, Stats))
    {
        SetIfChanged(Row.Record, MakeRecordText(PS, Stats));
        SetIfChanged(Row.Score, FString::FromInt(Stats.Score));
    }
    else
    {
        // 상대 팀 점수는 게임이 끝날 때까지 가린다(점수 변화로 위치를 짐작할 수 있어서)
        SetIfChanged(Row.Record, TEXT("게임이 끝나면 공개돼요"));
        SetIfChanged(Row.Score, TEXT("?"));
    }
}

bool UScoreboardWidget::GetVisibleStats(const AMyPlayerState* PS, FHideScoreStats& OutStats) const
{
    const AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer());
    return PC && PC->GetVisibleScoreStats(PS, OutStats);
}

void UScoreboardWidget::AddEmptyRow(const FString& Message)
{
    UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    T->SetText(FText::FromString(Message));
    T->SetFont(MakeFont(false, 14));
    T->SetColorAndOpacity(FSlateColor(SubLabelColor));
    if (UVerticalBoxSlot* S = Rows->AddChildToVerticalBox(T))
    {
        S->SetPadding(FMargin(16.f, 4.f));
    }
}

void UScoreboardWidget::RebuildRows()
{
    if (!Rows)
    {
        return;
    }

    const UWorld* World = GetWorld();
    const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
    const APlayerController* PC = GetOwningPlayer();
    const APlayerState* LocalPS = PC ? PC->PlayerState.Get() : nullptr;

    TArray<const AMyPlayerState*> Seekers;
    TArray<const AMyPlayerState*> Runners;
    if (GS)
    {
        for (const APlayerState* PS : GS->PlayerArray)
        {
            const AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
            if (!MyPS)
            {
                continue;
            }

            if (MyPS->GetFinalRole() == EFinalRole::Seeker)
            {
                Seekers.Add(MyPS);
            }
            else if (MyPS->GetFinalRole() == EFinalRole::Runner)
            {
                Runners.Add(MyPS);
            }
        }
    }

    // 보이는 점수(가려진 사람은 -1)
    TMap<const AMyPlayerState*, int32> VisibleScore;
    bool bAnyHidden = false;
    for (const TArray<const AMyPlayerState*>* Team : { &Seekers, &Runners })
    {
        for (const AMyPlayerState* PS : *Team)
        {
            FHideScoreStats Stats;
            const bool bKnown = GetVisibleStats(PS, Stats);
            VisibleScore.Add(PS, bKnown ? Stats.Score : -1);
            bAnyHidden |= !bKnown;
        }
    }

    // 점수 높은 순, 같으면(가려진 팀은 모두 같음) 이름 순
    auto ByScore = [&VisibleScore](const AMyPlayerState& A, const AMyPlayerState& B)
    {
        const int32 ScoreA = VisibleScore.FindRef(&A);
        const int32 ScoreB = VisibleScore.FindRef(&B);
        if (ScoreA != ScoreB)
        {
            return ScoreA > ScoreB;
        }
        return A.GetNickname() < B.GetNickname();
    };
    Seekers.Sort(ByScore);
    Runners.Sort(ByScore);

    TArray<const AMyPlayerState*> Order = Seekers;
    Order.Add(nullptr);
    Order.Append(Runners);

    // 사람이나 순위가 그대로면 줄 위젯은 두고 글자만 바꾼다(다시 만들면 한 프레임 비어 보여 깜빡인다)
    if (bRowsBuilt && Order == LastRowOrder && bAnyHidden == bLastAnyHidden && RowWidgets.Num() == Order.Num() - 1)
    {
        int32 RowIndex = 0;
        for (const AMyPlayerState* PS : Order)
        {
            if (PS)
            {
                FillRow(RowWidgets[RowIndex++], PS, PS == LocalPS);
            }
        }
        return;
    }

    bRowsBuilt = true;
    bLastAnyHidden = bAnyHidden;
    LastRowOrder = Order;
    RowWidgets.Reset();
    Rows->ClearChildren();

    auto AddTeam = [&](const FString& Title, const FLinearColor& Color, const TArray<const AMyPlayerState*>& Team)
    {
        AddTeamHeader(Title, Color, Team.Num());
        if (Team.Num() == 0)
        {
            AddEmptyRow(TEXT("아직 아무도 없어요"));
            return;
        }

        AddColumnHeader();
        for (int32 i = 0; i < Team.Num(); ++i)
        {
            const bool bLocal = Team[i] == LocalPS;
            // 점수가 가려진 팀은 순위도 보여 주지 않는다
            const bool bKnown = VisibleScore.FindRef(Team[i]) >= 0;
            const FRowWidgets Row = AddPlayerRow(bLocal, bKnown ? i + 1 : 0);
            FillRow(Row, Team[i], bLocal);
            RowWidgets.Add(Row);
        }
    };

    AddTeam(TEXT("술래"), SeekerColor, Seekers);
    AddTeam(TEXT("숨는 사람"), RunnerColor, Runners);

    UTextBlock* Footer = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Footer->SetText(FText::FromString(bAnyHidden
        ? TEXT("게임 중에는 상대 팀 점수가 가려져요 · 끝나면 모두 공개돼요")
        : TEXT("Tab을 누르고 있는 동안 보여요 · 점수는 판마다 새로 시작해요")));
    Footer->SetFont(MakeFont(false, 12));
    Footer->SetColorAndOpacity(FSlateColor(SubLabelColor));
    if (UVerticalBoxSlot* S = Rows->AddChildToVerticalBox(Footer))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
    }
}

// ===== 도움말 =====

UScoreboardWidget::EHelpGroup UScoreboardWidget::GetLocalHelpGroup() const
{
    const APlayerController* PC = GetOwningPlayer();
    const AMyPlayerState* PS = PC ? PC->GetPlayerState<AMyPlayerState>() : nullptr;
    if (!PS)
    {
        return EHelpGroup::Common;
    }

    if (PS->GetFinalRole() == EFinalRole::Seeker)
    {
        return EHelpGroup::Seeker;
    }

    return PS->IsRoundEliminated() ? EHelpGroup::Ghost : EHelpGroup::Runner;
}

void UScoreboardWidget::AddHelpGroup(EHelpGroup Group, const FString& Title, const FLinearColor& Color, const TArray<FHelpLine>& Lines)
{
    const bool bMine = Group == GetLocalHelpGroup();

    // 내 역할 묶음은 테두리를 그려 눈에 띄게
    UBorder* Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Box->SetBrush(bMine
        ? RoundBox(Color.CopyWithNewOpacity(0.10f), 14.f, Color, 2.f)
        : RoundBox(FLinearColor::Transparent, 14.f));
    Box->SetPadding(FMargin(12.f, 8.f, 12.f, 10.f));

    UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Box->SetContent(Inner);

    UHorizontalBox* TitleLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Chip->SetBrush(RoundBox(Color, 10.f));
    Chip->SetPadding(FMargin(10.f, 2.f, 10.f, 4.f));
    UTextBlock* ChipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    ChipText->SetText(FText::FromString(Title));
    ChipText->SetFont(MakeFont(true, 14));
    ChipText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Chip->SetContent(ChipText);
    TitleLine->AddChildToHorizontalBox(Chip);

    if (bMine)
    {
        UTextBlock* MineText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        MineText->SetText(FText::FromString(TEXT("지금 내 역할")));
        MineText->SetFont(MakeFont(true, 12));
        MineText->SetColorAndOpacity(FSlateColor(Color));
        if (UHorizontalBoxSlot* S = TitleLine->AddChildToHorizontalBox(MineText))
        {
            S->SetVerticalAlignment(VAlign_Center);
            S->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
        }
    }

    if (UVerticalBoxSlot* S = Inner->AddChildToVerticalBox(TitleLine))
    {
        S->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    for (const FHelpLine& L : Lines)
    {
        UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

        UTextBlock* Action = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Action->SetText(FText::FromString(L.Action));
        Action->SetFont(MakeFont(true, 15));
        Action->SetColorAndOpacity(FSlateColor(LabelColor));
        if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Action))
        {
            S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            S->SetVerticalAlignment(VAlign_Center);
        }

        UTextBlock* Points = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Points->SetText(FText::FromString(L.Points));
        Points->SetFont(MakeFont(true, 15));
        Points->SetColorAndOpacity(FSlateColor(AccentDark));
        if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Points))
        {
            S->SetVerticalAlignment(VAlign_Center);
            S->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
        }

        if (UVerticalBoxSlot* S = Inner->AddChildToVerticalBox(Line))
        {
            S->SetPadding(FMargin(4.f, 3.f, 0.f, 0.f));
        }

        if (!L.Detail.IsEmpty())
        {
            UTextBlock* Detail = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
            Detail->SetText(FText::FromString(L.Detail));
            Detail->SetFont(MakeFont(false, 12));
            Detail->SetColorAndOpacity(FSlateColor(SubLabelColor));
            Detail->SetAutoWrapText(true);
            if (UVerticalBoxSlot* S = Inner->AddChildToVerticalBox(Detail))
            {
                S->SetPadding(FMargin(4.f, 0.f, 0.f, 0.f));
            }
        }
    }

    if (UVerticalBoxSlot* S = HelpBody->AddChildToVerticalBox(Box))
    {
        S->SetPadding(FMargin(0.f, 3.f));
    }
}

void UScoreboardWidget::RebuildHelp()
{
    if (!HelpBody)
    {
        return;
    }

    HelpBody->ClearChildren();

    using PS = AMyPlayerState;

    AddHelpGroup(EHelpGroup::Seeker, TEXT("술래"), SeekerColor, {
        { TEXT("숨는 사람 맞히기"), FString::Printf(TEXT("+%d"), PS::ScoreSeekerHit), TEXT("") },
        { TEXT("탈락시키기"), FString::Printf(TEXT("+%d 추가"), PS::ScoreSeekerEliminate), TEXT("") },
    });

    AddHelpGroup(EHelpGroup::Runner, TEXT("숨는 사람"), RunnerColor, {
        { TEXT("단서 줍기"), FString::Printf(TEXT("+%d"), PS::ScoreClue), TEXT("") },
        { TEXT("코앞에서 숨기"), FString::Printf(TEXT("1초 +%d"), PS::ScoreCloseHidePerSecond),
            FString::Printf(TEXT("물체로 숨은 채 5m 안의 술래에게 보이는 곳에 있을 때 (최대 %d점)"), PS::MaxCloseHidePoints) },
        { TEXT("아슬아슬 회피"), FString::Printf(TEXT("+%d"), PS::ScoreNearMiss),
            FString::Printf(TEXT("술래의 마법이 2m 안에 빗나갔을 때 (최대 %d번)"), PS::MaxNearMisses) },
    });

    AddHelpGroup(EHelpGroup::Ghost, TEXT("유령"), GhostColor, {
        { TEXT("속임수 성공"), FString::Printf(TEXT("+%d"), PS::ScoreDecoyHit),
            FString::Printf(TEXT("내가 놓은 물체를 술래가 부쉈을 때 (최대 %d번)"), PS::MaxDecoyHits) },
    });

    AddHelpGroup(EHelpGroup::Common, TEXT("공통"), CommonColor, {
        { TEXT("비밀번호 맞히기"), FString::Printf(TEXT("+%d"), PS::ScorePassword), TEXT("") },
        { TEXT("이긴 팀"), FString::Printf(TEXT("+%d"), PS::ScoreWin), TEXT("유령이 된 사람도 팀이 이기면 받아요") },
    });

    UTextBlock* Note = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Note->SetText(FText::FromString(TEXT("멀리 도망만 다니면 점수가 없어요.\n피버 타임에는 숨는 사람 위치에서 주기적으로 호루라기 소리가 나요.")));
    Note->SetFont(MakeFont(false, 12));
    Note->SetColorAndOpacity(FSlateColor(SubLabelColor));
    Note->SetAutoWrapText(true);
    if (UVerticalBoxSlot* S = HelpBody->AddChildToVerticalBox(Note))
    {
        S->SetPadding(FMargin(6.f, 8.f, 6.f, 0.f));
    }
}
