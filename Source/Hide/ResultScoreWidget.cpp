#include "ResultScoreWidget.h"

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

    // 설정 창 / 점수판과 같은 색
    const FLinearColor PanelOuterFill = Hex(255, 236, 196);
    const FLinearColor PanelOuterLine = Hex(236, 152, 42);
    const FLinearColor PanelInnerFill = Hex(255, 252, 242);
    const FLinearColor PanelInnerLine = Hex(240, 196, 128);
    const FLinearColor RibbonFill = Hex(146, 92, 232);
    const FLinearColor RibbonLine = Hex(92, 46, 170);
    const FLinearColor LabelColor = Hex(122, 74, 30);
    const FLinearColor SubLabelColor = Hex(122, 74, 30, 160);
    const FLinearColor AccentDark = Hex(214, 82, 90);
    const FLinearColor SeekerColor = Hex(214, 64, 70);
    const FLinearColor RunnerColor = Hex(56, 150, 96);
    const FLinearColor GoldFill = Hex(255, 214, 96);
    const FLinearColor GoldLine = Hex(214, 150, 30);
    const FLinearColor RowMineFill = Hex(255, 214, 200);
    const FLinearColor RowMineLine = Hex(255, 128, 128);

    constexpr float CardWidth = 470.f;

    // 결과 영상이 이만큼 나온 뒤 카드가 나타난다
    constexpr float ShowDelay = 2.5f;
    constexpr float SlideTime = 0.45f;

    // 카드 폭 + 오른쪽 여백보다 크게 해서 화면 밖에서 출발한다
    constexpr float SlideDistance = 620.f;

    // 내용을 만든 뒤 나타나기까지 최소 간격(레이아웃이 자리 잡을 시간)
    constexpr float PrepareTime = 0.2f;

    // 점수 공개 복제를 이 시간까지 기다리고, 넘으면 받은 것만으로 그린다
    constexpr float RevealWaitLimit = 5.f;

    FSlateBrush RoundBox(const FLinearColor& Fill, float Radius, const FLinearColor& Line = FLinearColor::Transparent, float LineWidth = 0.f)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }
}

TSharedRef<SWidget> UResultScoreWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildShell();
    }

    return Super::RebuildWidget();
}

FSlateFontInfo UResultScoreWidget::MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor, int32 OutlineSize) const
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

void UResultScoreWidget::BuildShell()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    LightFont = LoadObject<UObject>(nullptr, LightFontPath);

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
    WidgetTree->RootWidget = Root;

    // 화면 오른쪽 가운데. 아래쪽 복귀 카운트다운과 겹치지 않게 위로 조금 올린다.
    Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardWrap"));
    Card->SetBrushColor(FLinearColor::Transparent);
    Card->SetPadding(FMargin(0.f));
    if (UOverlaySlot* S = Root->AddChildToOverlay(Card))
    {
        S->SetHorizontalAlignment(HAlign_Right);
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(0.f, 0.f, 48.f, 90.f));
    }

    UOverlay* CardOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Card"));
    Card->SetContent(CardOverlay);

    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelOuter"));
    Outer->SetBrush(RoundBox(PanelOuterFill, 26.f, PanelOuterLine, 4.f));
    Outer->SetPadding(FMargin(8.f));
    if (UOverlaySlot* S = CardOverlay->AddChildToOverlay(Outer))
    {
        S->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
    }

    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelInner"));
    Inner->SetBrush(RoundBox(PanelInnerFill, 20.f, PanelInnerLine, 2.f));
    Inner->SetPadding(FMargin(22.f, 38.f, 22.f, 18.f));
    Outer->SetContent(Inner);

    USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Width"));
    Width->SetWidthOverride(CardWidth);
    Inner->SetContent(Width);

    Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Body"));
    Width->SetContent(Body);

    UBorder* Ribbon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Ribbon"));
    Ribbon->SetBrush(RoundBox(RibbonFill, 14.f, RibbonLine, 3.f));
    Ribbon->SetPadding(FMargin(36.f, 6.f, 36.f, 8.f));
    if (UOverlaySlot* S = CardOverlay->AddChildToOverlay(Ribbon))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Top);
    }

    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
    Title->SetText(FText::FromString(TEXT("이번 판 결과")));
    Title->SetFont(MakeFont(true, 24, RibbonLine, 2));
    Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Ribbon->SetContent(Title);

    // 처음에는 숨겨 두고 결과 영상이 잠깐 나온 뒤 화면 오른쪽 밖에서 미끄러져 들어온다.
    // (투명도로 페이드하면 둥근 상자 테두리 선만 투명도를 따르지 않아 테두리만 먼저 보인다)
    // 버튼(복귀 카운트다운)을 가리지 않게 클릭은 통과.
    Card->SetVisibility(ESlateVisibility::Hidden);
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UResultScoreWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    Elapsed += InDeltaTime;

    // 내용은 결과 화면이 뜨자마자(점수 공개가 도착하는 대로) 숨긴 채로 미리 만들어 둔다
    if (!bContentBuilt)
    {
        bContentBuilt = TryBuildContent();
        if (bContentBuilt)
        {
            BuiltTime = Elapsed;
        }
    }

    if (Card && bContentBuilt)
    {
        const float SlideStart = FMath::Max(ShowDelay, BuiltTime + PrepareTime);
        if (Elapsed < SlideStart)
        {
            return;
        }

        // 화면 오른쪽 밖에서 들어오며 끝에서 살짝 부드럽게 멈춘다
        const float T = FMath::Clamp((Elapsed - SlideStart) / SlideTime, 0.f, 1.f);
        const float Ease = 1.f - FMath::Pow(1.f - T, 3.f);
        Card->SetRenderTranslation(FVector2D(SlideDistance * (1.f - Ease), 0.f));
        if (!Card->IsVisible())
        {
            Card->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }
}

bool UResultScoreWidget::TryBuildContent()
{
    const UWorld* World = GetWorld();
    const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
    if (!GS || !Body)
    {
        return false;
    }

    const APlayerController* PC = GetOwningPlayer();
    const APlayerState* LocalPS = PC ? PC->PlayerState.Get() : nullptr;

    TArray<FResultEntry> Entries;
    bool bAllRevealed = true;
    for (const APlayerState* PS : GS->PlayerArray)
    {
        const AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
        if (!MyPS || MyPS->GetFinalRole() == EFinalRole::None)
        {
            continue;
        }

        if (!MyPS->AreScoresRevealed())
        {
            bAllRevealed = false;
        }

        FResultEntry& Entry = Entries.AddDefaulted_GetRef();
        Entry.PS = MyPS;
        Entry.Stats = MyPS->AreScoresRevealed() ? MyPS->GetRevealedScoreStats()
            : (MyPS == LocalPS ? MyPS->GetScoreStats() : FHideScoreStats());
        Entry.Name = MyPS->GetNickname().IsEmpty() ? MyPS->GetPlayerName() : MyPS->GetNickname();
        Entry.bLocal = MyPS == LocalPS;
    }

    // 점수 공개가 아직 복제되지 않았으면 조금 더 기다린다
    if (!bAllRevealed && Elapsed < RevealWaitLimit)
    {
        return false;
    }

    AssignTitles(Entries);

    Body->ClearChildren();

    // MVP: 점수가 가장 높은 사람(같으면 이긴 팀, 그다음 이름 순)
    const FResultEntry* Mvp = nullptr;
    for (const FResultEntry& E : Entries)
    {
        if (!Mvp)
        {
            Mvp = &E;
            continue;
        }

        const bool bEWon = E.PS->GetFinalRole() == WinningRole;
        const bool bMvpWon = Mvp->PS->GetFinalRole() == WinningRole;
        if (E.Stats.Score > Mvp->Stats.Score
            || (E.Stats.Score == Mvp->Stats.Score && bEWon && !bMvpWon)
            || (E.Stats.Score == Mvp->Stats.Score && bEWon == bMvpWon && E.Name < Mvp->Name))
        {
            Mvp = &E;
        }
    }

    if (Mvp && Mvp->Stats.Score > 0)
    {
        AddMvp(*Mvp);
    }

    TArray<FResultEntry> Seekers;
    TArray<FResultEntry> Runners;
    for (const FResultEntry& E : Entries)
    {
        (E.PS->GetFinalRole() == EFinalRole::Seeker ? Seekers : Runners).Add(E);
    }

    auto ByScore = [](const FResultEntry& A, const FResultEntry& B)
    {
        return A.Stats.Score != B.Stats.Score ? A.Stats.Score > B.Stats.Score : A.Name < B.Name;
    };
    Seekers.Sort(ByScore);
    Runners.Sort(ByScore);

    // 이긴 팀을 위에
    if (WinningRole == EFinalRole::Runner)
    {
        AddTeam(TEXT("숨는 사람"), RunnerColor, true, Runners);
        AddTeam(TEXT("술래"), SeekerColor, false, Seekers);
    }
    else
    {
        AddTeam(TEXT("술래"), SeekerColor, WinningRole == EFinalRole::Seeker, Seekers);
        AddTeam(TEXT("숨는 사람"), RunnerColor, false, Runners);
    }

    return true;
}

void UResultScoreWidget::AssignTitles(TArray<FResultEntry>& Entries) const
{
    struct FTitleRule
    {
        const TCHAR* Title;
        EFinalRole Role;   // None이면 역할 무관
        TFunction<int32(const FHideScoreStats&)> Value;
    };

    // 위에 있을수록 먼저 붙인다. 한 사람에게는 칭호 하나만, 각 칭호는 그 기록이 가장 높은 한 명에게.
    const FTitleRule Rules[] = {
        { TEXT("비밀번호 해커"), EFinalRole::None, [](const FHideScoreStats& S) { return S.bPasswordSolved ? 1 : 0; } },
        { TEXT("사냥꾼"), EFinalRole::Seeker, [](const FHideScoreStats& S) { return S.Eliminations; } },
        { TEXT("명사수"), EFinalRole::Seeker, [](const FHideScoreStats& S) { return S.Hits; } },
        { TEXT("코앞 숨기 장인"), EFinalRole::Runner, [](const FHideScoreStats& S) { return S.CloseHidePoints; } },
        { TEXT("아슬아슬 회피왕"), EFinalRole::Runner, [](const FHideScoreStats& S) { return S.NearMisses; } },
        { TEXT("단서 수집가"), EFinalRole::Runner, [](const FHideScoreStats& S) { return S.Clues; } },
        { TEXT("속임수의 달인"), EFinalRole::Runner, [](const FHideScoreStats& S) { return S.DecoyHits; } },
    };

    for (const FTitleRule& Rule : Rules)
    {
        FResultEntry* Best = nullptr;
        int32 BestValue = 0;
        for (FResultEntry& E : Entries)
        {
            if (!E.Title.IsEmpty() || (Rule.Role != EFinalRole::None && E.PS->GetFinalRole() != Rule.Role))
            {
                continue;
            }

            const int32 V = Rule.Value(E.Stats);
            if (V > BestValue || (V == BestValue && V > 0 && Best && E.Stats.Score > Best->Stats.Score))
            {
                Best = &E;
                BestValue = V;
            }
        }

        if (Best && BestValue > 0)
        {
            Best->Title = Rule.Title;
        }
    }

    // 칭호가 없는 사람 중 끝까지 살아남은 숨는 사람
    for (FResultEntry& E : Entries)
    {
        if (E.Title.IsEmpty() && E.PS->GetFinalRole() == EFinalRole::Runner && !E.PS->IsRoundEliminated())
        {
            E.Title = TEXT("끝까지 생존");
        }
    }
}

UWidget* UResultScoreWidget::MakeNameLine(const FString& Name, bool bLocal, int32 Size, const FLinearColor& Color)
{
    // 이름이 길면 "…"으로 줄이고, "나" 표시는 이름과 따로 둬서 잘리지 않게 한다
    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Line->SetClipping(EWidgetClipping::ClipToBounds);

    UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    NameText->SetText(FText::FromString(Name));
    NameText->SetFont(MakeFont(true, Size));
    NameText->SetColorAndOpacity(FSlateColor(Color));
    NameText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(NameText))
    {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (bLocal)
    {
        UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Chip->SetBrush(RoundBox(AccentDark, 8.f));
        Chip->SetPadding(FMargin(7.f, 1.f, 7.f, 3.f));
        UTextBlock* ChipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        ChipText->SetText(FText::FromString(TEXT("나")));
        ChipText->SetFont(MakeFont(true, 12));
        ChipText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Chip->SetContent(ChipText);
        if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Chip))
        {
            S->SetVerticalAlignment(VAlign_Center);
            S->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
        }
    }

    return Line;
}

void UResultScoreWidget::AddMvp(const FResultEntry& Mvp)
{
    const bool bSeeker = Mvp.PS->GetFinalRole() == EFinalRole::Seeker;
    const FLinearColor TeamColor = bSeeker ? SeekerColor : RunnerColor;

    UBorder* Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Box->SetBrush(RoundBox(GoldFill.CopyWithNewOpacity(0.35f), 16.f, GoldLine, 3.f));
    Box->SetPadding(FMargin(16.f, 10.f, 16.f, 12.f));

    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Box->SetContent(Line);

    // MVP 배지
    UBorder* Badge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Badge->SetBrush(RoundBox(GoldFill, 12.f, GoldLine, 2.f));
    Badge->SetPadding(FMargin(12.f, 4.f, 12.f, 6.f));
    UTextBlock* BadgeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    BadgeText->SetText(FText::FromString(TEXT("MVP")));
    BadgeText->SetFont(MakeFont(true, 22, GoldLine, 2));
    BadgeText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Badge->SetContent(BadgeText);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Badge))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Info))
    {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(14.f, 0.f, 8.f, 0.f));
    }

    Info->AddChildToVerticalBox(MakeNameLine(Mvp.Name, Mvp.bLocal, 22, LabelColor));

    FString Sub = bSeeker ? TEXT("술래") : TEXT("숨는 사람");
    if (!Mvp.Title.IsEmpty())
    {
        Sub += TEXT(" · ") + Mvp.Title;
    }
    UTextBlock* SubText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    SubText->SetText(FText::FromString(Sub));
    SubText->SetFont(MakeFont(true, 14));
    SubText->SetColorAndOpacity(FSlateColor(TeamColor));
    Info->AddChildToVerticalBox(SubText);

    UTextBlock* Score = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Score->SetText(FText::FromString(FString::Printf(TEXT("%d점"), Mvp.Stats.Score)));
    Score->SetFont(MakeFont(true, 26));
    Score->SetColorAndOpacity(FSlateColor(AccentDark));
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Score))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (UVerticalBoxSlot* S = Body->AddChildToVerticalBox(Box))
    {
        S->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    }
}

void UResultScoreWidget::AddTeam(const FString& TeamName, const FLinearColor& Color, bool bWon, const TArray<FResultEntry>& Team)
{
    UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Chip->SetBrush(RoundBox(Color, 12.f));
    Chip->SetPadding(FMargin(14.f, 3.f, 14.f, 5.f));
    UTextBlock* ChipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    ChipText->SetText(FText::FromString(TeamName));
    ChipText->SetFont(MakeFont(true, 16));
    ChipText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Chip->SetContent(ChipText);
    if (UHorizontalBoxSlot* S = Header->AddChildToHorizontalBox(Chip))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    UTextBlock* ResultText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    ResultText->SetText(FText::FromString(bWon ? TEXT("승리!") : TEXT("패배")));
    ResultText->SetFont(MakeFont(true, 16));
    ResultText->SetColorAndOpacity(FSlateColor(bWon ? GoldLine : SubLabelColor));
    if (UHorizontalBoxSlot* S = Header->AddChildToHorizontalBox(ResultText))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
    }

    if (UVerticalBoxSlot* S = Body->AddChildToVerticalBox(Header))
    {
        S->SetPadding(FMargin(0.f, 10.f, 0.f, 4.f));
    }

    if (Team.Num() == 0)
    {
        UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Empty->SetText(FText::FromString(TEXT("아무도 없었어요")));
        Empty->SetFont(MakeFont(false, 14));
        Empty->SetColorAndOpacity(FSlateColor(SubLabelColor));
        if (UVerticalBoxSlot* S = Body->AddChildToVerticalBox(Empty))
        {
            S->SetPadding(FMargin(12.f, 2.f));
        }
        return;
    }

    for (int32 i = 0; i < Team.Num(); ++i)
    {
        AddRow(Team[i], i + 1);
    }
}

void UResultScoreWidget::AddRow(const FResultEntry& Entry, int32 Rank)
{
    UBorder* RowBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    if (Entry.bLocal)
    {
        RowBg->SetBrush(RoundBox(RowMineFill, 12.f, RowMineLine, 2.f));
    }
    else
    {
        RowBg->SetBrushColor(FLinearColor::Transparent);
    }
    RowBg->SetPadding(FMargin(10.f, 4.f, 12.f, 6.f));

    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    RowBg->SetContent(Line);

    USizeBox* RankBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    RankBox->SetWidthOverride(34.f);
    UTextBlock* RankText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    RankText->SetText(FText::FromString(FString::FromInt(Rank)));
    RankText->SetFont(MakeFont(true, 16));
    RankText->SetColorAndOpacity(FSlateColor(SubLabelColor));
    RankText->SetJustification(ETextJustify::Center);
    RankBox->SetContent(RankText);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(RankBox))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    // 이름 + 칭호(이름 아래 작은 글씨)
    UVerticalBox* NameBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    NameBox->SetClipping(EWidgetClipping::ClipToBounds);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(NameBox))
    {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(6.f, 0.f, 8.f, 0.f));
    }

    NameBox->AddChildToVerticalBox(MakeNameLine(Entry.Name, Entry.bLocal, 17, Entry.bLocal ? AccentDark : LabelColor));

    if (!Entry.Title.IsEmpty())
    {
        UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Title->SetText(FText::FromString(Entry.Title));
        Title->SetFont(MakeFont(true, 12));
        Title->SetColorAndOpacity(FSlateColor(GoldLine));
        NameBox->AddChildToVerticalBox(Title);
    }

    UTextBlock* Score = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Score->SetText(FText::FromString(FString::FromInt(Entry.Stats.Score)));
    Score->SetFont(MakeFont(true, 20));
    Score->SetColorAndOpacity(FSlateColor(Entry.bLocal ? AccentDark : LabelColor));
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Score))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (UVerticalBoxSlot* S = Body->AddChildToVerticalBox(RowBg))
    {
        S->SetPadding(FMargin(0.f, 1.f));
    }
}
