#include "ControlsGuideWidget.h"

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
#include "Styling/CoreStyle.h"

namespace
{
    const TCHAR* BoldFontPath = TEXT("/Game/Game/Fonts/Maplestory_Bold_Font.Maplestory_Bold_Font");
    const TCHAR* LightFontPath = TEXT("/Game/Game/Fonts/Maplestory_Light_Font.Maplestory_Light_Font");

    FLinearColor Hex(uint8 R, uint8 G, uint8 B, uint8 A = 255)
    {
        return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
    }

    // 볼륨 설정 창과 같은 톤
    const FLinearColor CardFill = Hex(255, 252, 242, 235);
    const FLinearColor CardLine = Hex(236, 152, 42);
    const FLinearColor LabelColor = Hex(122, 74, 30);
    const FLinearColor KeyFill = Hex(255, 255, 255);
    const FLinearColor KeyLine = Hex(200, 160, 110);

    FSlateBrush RoundBox(const FLinearColor& Fill, float Radius, const FLinearColor& Line = FLinearColor::Transparent, float LineWidth = 0.f)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }

    constexpr float AutoCollapseSeconds = 12.f;
}

TSharedRef<SWidget> UControlsGuideWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

FSlateFontInfo UControlsGuideWidget::MakeFont(bool bBold, int32 Size) const
{
    const UObject* FontObj = bBold ? BoldFont.Get() : LightFont.Get();
    if (FontObj)
    {
        return FSlateFontInfo(FontObj, Size);
    }
    return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
}

void UControlsGuideWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    LightFont = LoadObject<UObject>(nullptr, LightFontPath);

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
    WidgetTree->RootWidget = Root;

    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));
    if (UOverlaySlot* S = Root->AddChildToOverlay(Stack))
    {
        S->SetHorizontalAlignment(HAlign_Left);
        S->SetVerticalAlignment(VAlign_Bottom);
        S->SetPadding(FMargin(24.f, 0.f, 0.f, 24.f));
    }

    // 펼친 카드
    Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
    Card->SetBrush(RoundBox(CardFill, 18.f, CardLine, 3.f));
    Card->SetPadding(FMargin(18.f, 14.f, 22.f, 14.f));
    if (UVerticalBoxSlot* S = Stack->AddChildToVerticalBox(Card))
    {
        S->SetHorizontalAlignment(HAlign_Left);
    }

    UVerticalBox* CardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardBox"));
    Card->SetContent(CardBox);

    TitleRibbon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TitleRibbon"));
    TitleRibbon->SetPadding(FMargin(16.f, 4.f, 16.f, 6.f));
    if (UVerticalBoxSlot* S = CardBox->AddChildToVerticalBox(TitleRibbon))
    {
        S->SetHorizontalAlignment(HAlign_Left);
        S->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
    }

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetFont(MakeFont(true, 18));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    TitleRibbon->SetContent(TitleText);

    Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Rows"));
    CardBox->AddChildToVerticalBox(Rows);

    UTextBlock* Footer = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Footer"));
    Footer->SetText(FText::FromString(TEXT("H 조작법 열기/닫기 · F10 설정")));
    Footer->SetFont(MakeFont(false, 12));
    Footer->SetColorAndOpacity(FSlateColor(LabelColor.CopyWithNewOpacity(0.6f)));
    if (UVerticalBoxSlot* S = CardBox->AddChildToVerticalBox(Footer))
    {
        S->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
    }

    // 접혔을 때 보이는 작은 버튼 모양
    Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Chip"));
    Chip->SetBrush(RoundBox(CardFill, 14.f, CardLine, 2.f));
    Chip->SetPadding(FMargin(12.f, 5.f, 14.f, 6.f));
    UTextBlock* ChipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ChipText"));
    ChipText->SetText(FText::FromString(TEXT("H  조작법")));
    ChipText->SetFont(MakeFont(true, 14));
    ChipText->SetColorAndOpacity(FSlateColor(LabelColor));
    Chip->SetContent(ChipText);
    if (UVerticalBoxSlot* S = Stack->AddChildToVerticalBox(Chip))
    {
        S->SetHorizontalAlignment(HAlign_Left);
    }

    // 클릭을 막지 않도록
    SetVisibility(ESlateVisibility::HitTestInvisible);

    RebuildRows();
    SetExpanded(bExpanded);
}

void UControlsGuideWidget::AddRow(const FString& Key, const FString& Desc)
{
    UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    // 키 모양 칩
    USizeBox* KeySize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    KeySize->SetMinDesiredWidth(96.f);
    UBorder* KeyChip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    KeyChip->SetBrush(RoundBox(KeyFill, 8.f, KeyLine, 2.f));
    KeyChip->SetPadding(FMargin(8.f, 2.f, 8.f, 4.f));
    KeyChip->SetHorizontalAlignment(HAlign_Center);
    UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    KeyText->SetText(FText::FromString(Key));
    KeyText->SetFont(MakeFont(true, 14));
    KeyText->SetColorAndOpacity(FSlateColor(LabelColor));
    KeyChip->SetContent(KeyText);
    KeySize->SetContent(KeyChip);
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(KeySize))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    UTextBlock* DescText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    DescText->SetText(FText::FromString(Desc));
    DescText->SetFont(MakeFont(false, 15));
    DescText->SetColorAndOpacity(FSlateColor(LabelColor));
    if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(DescText))
    {
        S->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (UVerticalBoxSlot* S = Rows->AddChildToVerticalBox(Line))
    {
        S->SetPadding(FMargin(0.f, 3.f));
    }
}

void UControlsGuideWidget::RebuildRows()
{
    if (!Rows)
    {
        return;
    }

    Rows->ClearChildren();

    FString Title;
    FLinearColor RibbonColor = Hex(146, 92, 232);

    switch (Role)
    {
    case EGuideRole::Runner:
        Title = TEXT("러너 조작법");
        RibbonColor = Hex(64, 120, 230);
        AddRow(TEXT("WASD"), TEXT("이동"));
        AddRow(TEXT("Space"), TEXT("점프"));
        AddRow(TEXT("F / 우클릭"), TEXT("물체에 숨기 / 나오기"));
        AddRow(TEXT("T"), TEXT("숨은 채로 제자리 고정"));
        AddRow(TEXT("Q / E"), TEXT("고정 중 회전"));
        AddRow(TEXT("R"), TEXT("숨은 물체 똑바로 세우기"));
        AddRow(TEXT("우클릭"), TEXT("문 앞에서 비밀번호 입력"));
        break;

    case EGuideRole::Seeker:
        Title = TEXT("술래 조작법");
        RibbonColor = Hex(214, 64, 70);
        AddRow(TEXT("WASD"), TEXT("이동"));
        AddRow(TEXT("Space"), TEXT("점프"));
        AddRow(TEXT("좌클릭"), TEXT("마법 발사"));
        AddRow(TEXT("좌클릭 꾹"), TEXT("조준한 물체 파괴"));
        break;

    case EGuideRole::Ghost:
        Title = TEXT("유령 조작법");
        RibbonColor = Hex(146, 92, 232);
        AddRow(TEXT("WASD"), TEXT("이동"));
        AddRow(TEXT("Space / Ctrl"), TEXT("위로 / 아래로"));
        AddRow(TEXT("아이콘 클릭"), TEXT("놓을 물체 고르기"));
        AddRow(TEXT("좌클릭"), TEXT("물체 놓기"));
        AddRow(TEXT("우클릭"), TEXT("놓기 취소"));
        AddRow(TEXT("Q / E"), TEXT("놓을 물체 회전"));
        AddRow(TEXT("R"), TEXT("물체 목록 새로고침"));
        break;

    default:
        break;
    }

    if (TitleText)
    {
        TitleText->SetText(FText::FromString(Title));
    }
    if (TitleRibbon)
    {
        TitleRibbon->SetBrush(RoundBox(RibbonColor, 12.f));
    }
}

void UControlsGuideWidget::SetRole(EGuideRole InRole)
{
    if (Role == InRole)
    {
        return;
    }

    Role = InRole;
    RebuildRows();

    // 역할이 정해지거나 바뀌면 한 번 펼쳐서 보여준다
    SetExpanded(Role != EGuideRole::None);
}

void UControlsGuideWidget::ToggleExpanded()
{
    if (Role == EGuideRole::None)
    {
        return;
    }

    SetExpanded(!bExpanded);
    AutoCollapseRemain = 0.f; // 직접 연 건 자동으로 접지 않는다
}

void UControlsGuideWidget::SetExpanded(bool bInExpanded)
{
    bExpanded = bInExpanded;
    AutoCollapseRemain = bExpanded ? AutoCollapseSeconds : 0.f;

    const bool bHasRole = Role != EGuideRole::None;
    if (Card)
    {
        Card->SetVisibility(bHasRole && bExpanded ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (Chip)
    {
        Chip->SetVisibility(bHasRole && !bExpanded ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void UControlsGuideWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bExpanded && AutoCollapseRemain > 0.f)
    {
        AutoCollapseRemain -= InDeltaTime;
        if (AutoCollapseRemain <= 0.f)
        {
            SetExpanded(false);
        }
    }
}
