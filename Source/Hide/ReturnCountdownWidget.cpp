#include "ReturnCountdownWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "MyGameState.h"
#include "MyPlayerController.h"
#include "MyPlayerState.h"

namespace
{
    const TCHAR* BoldFontPath = TEXT("/Game/Game/Fonts/Maplestory_Bold_Font.Maplestory_Bold_Font");
    const TCHAR* LightFontPath = TEXT("/Game/Game/Fonts/Maplestory_Light_Font.Maplestory_Light_Font");

    FLinearColor Hex(uint8 R, uint8 G, uint8 B, uint8 A = 255)
    {
        return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
    }

    // 조작법 안내/볼륨 설정 창과 같은 톤
    const FLinearColor CardFill = Hex(255, 252, 242, 235);
    const FLinearColor CardLine = Hex(236, 152, 42);
    const FLinearColor LabelColor = Hex(122, 74, 30);
    const FLinearColor ButtonFill = Hex(236, 152, 42);
    const FLinearColor ButtonHover = Hex(246, 172, 70);
    const FLinearColor ButtonPressed = Hex(206, 128, 30);

    FSlateBrush RoundBox(const FLinearColor& Fill, float Radius, const FLinearColor& Line = FLinearColor::Transparent, float LineWidth = 0.f)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }
}

TSharedRef<SWidget> UReturnCountdownWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

FSlateFontInfo UReturnCountdownWidget::MakeFont(bool bBold, int32 Size) const
{
    const UObject* FontObj = bBold ? BoldFont.Get() : LightFont.Get();
    if (FontObj)
    {
        return FSlateFontInfo(FontObj, Size);
    }
    return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
}

void UReturnCountdownWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    LightFont = LoadObject<UObject>(nullptr, LightFontPath);

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
    WidgetTree->RootWidget = Root;

    UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
    Card->SetBrush(RoundBox(CardFill, 18.f, CardLine, 3.f));
    Card->SetPadding(FMargin(24.f, 12.f, 24.f, 14.f));
    if (UOverlaySlot* S = Root->AddChildToOverlay(Card))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetVerticalAlignment(VAlign_Bottom);
        S->SetPadding(FMargin(0.f, 0.f, 0.f, 48.f));
    }

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
    Card->SetContent(Box);

    CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountdownText"));
    CountdownText->SetFont(MakeFont(true, 18));
    CountdownText->SetColorAndOpacity(FSlateColor(LabelColor));
    CountdownText->SetJustification(ETextJustify::Center);
    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(CountdownText))
    {
        S->SetHorizontalAlignment(HAlign_Center);
    }

    ReturnNowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ReturnNowButton"));
    FButtonStyle Style = ReturnNowButton->GetStyle();
    Style.SetNormal(RoundBox(ButtonFill, 12.f));
    Style.SetHovered(RoundBox(ButtonHover, 12.f));
    Style.SetPressed(RoundBox(ButtonPressed, 12.f));
    Style.SetDisabled(RoundBox(ButtonFill.CopyWithNewOpacity(0.4f), 12.f));
    Style.SetNormalPadding(FMargin(18.f, 6.f, 18.f, 8.f));
    Style.SetPressedPadding(FMargin(18.f, 7.f, 18.f, 7.f));
    ReturnNowButton->SetStyle(Style);
    ReturnNowButton->OnClicked.AddDynamic(this, &UReturnCountdownWidget::OnReturnNowClicked);
    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(ReturnNowButton))
    {
        S->SetHorizontalAlignment(HAlign_Center);
        S->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
    }

    ReturnNowText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReturnNowText"));
    ReturnNowText->SetText(FText::FromString(TEXT("바로 대기실로")));
    ReturnNowText->SetFont(MakeFont(true, 16));
    ReturnNowText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    ReturnNowButton->SetContent(ReturnNowText);

    // 카드 밖은 클릭이 아래 결과 화면으로 지나가게 한다
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    Refresh();
}

void UReturnCountdownWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    Refresh();
}

void UReturnCountdownWidget::Refresh()
{
    if (!CountdownText || !ReturnNowButton)
    {
        return;
    }

    const APlayerController* PC = GetOwningPlayer();
    const AMyPlayerState* PS = PC ? PC->GetPlayerState<AMyPlayerState>() : nullptr;
    const bool bIsHost = PS && PS->IsHost();
    ReturnNowButton->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    ReturnNowButton->SetIsEnabled(!bRequested);

    const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    const bool bEnded = GS && GS->GetGamePhase() == EGamePhase::Ended;
    const double Remaining = bEnded ? GS->GetPhaseRemainingSeconds() : 0.0;

    // 자동 복귀가 꺼져 있으면(남은 시간 0) 방장을 기다린다는 안내만 보여준다
    int32 Second = Remaining > 0.0 ? FMath::CeilToInt(static_cast<float>(Remaining)) : -1;
    if (bRequested)
    {
        Second = 0;
    }

    if (Second == LastShownSecond)
    {
        return;
    }
    LastShownSecond = Second;

    FString Message;
    if (Second == 0)
    {
        Message = TEXT("대기실로 이동하는 중...");
    }
    else if (Second > 0)
    {
        Message = FString::Printf(TEXT("%d초 후 대기실로 돌아갑니다"), Second);
    }
    else
    {
        Message = bIsHost ? TEXT("준비되면 대기실로 돌아가세요") : TEXT("방장이 대기실로 이동하면 함께 돌아갑니다");
    }
    CountdownText->SetText(FText::FromString(Message));
}

void UReturnCountdownWidget::OnReturnNowClicked()
{
    if (bRequested)
    {
        return;
    }

    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
    {
        bRequested = true;
        PC->RequestReturnToWaitRoomNow();
        Refresh();
    }
}
