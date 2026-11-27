#include "ReadyButtonWidget.h"

#include "MyPlayerController.h"
#include "MyPlayerState.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    const TCHAR* BoldFontPath = TEXT("/Game/Game/Fonts/Maplestory_Bold_Font.Maplestory_Bold_Font");
    const TCHAR* ClickSoundPath = TEXT("/Game/Game/Sounds/UI/SFX_UI_Click.SFX_UI_Click");
    const TCHAR* HoverSoundPath = TEXT("/Game/Game/Sounds/UI/SFX_UI_Hover.SFX_UI_Hover");

    // START 버튼 그림 크기
    const FVector2D ButtonImageSize(1374.0, 244.0);

    // 버튼이 놓이는 상자 크기(START와 같은 8:1). 글자 크기는 이 높이 기준.
    const FVector2D LayoutSize(800.0, 100.0);

    // START 그림에서 글자 높이는 버튼 높이의 약 43%(106/244). 같은 높이가 되는 글자 크기.
    constexpr int32 LabelFontSize = 38;
    constexpr int32 LabelOutlineSize = 2;

    // START 글자와 같은 느낌: 크림색 글자 + 진한 빨강 테두리
    const FLinearColor TextFill = FLinearColor::FromSRGBColor(FColor(255, 250, 236));
    const FLinearColor TextOutline = FLinearColor::FromSRGBColor(FColor(200, 52, 58));

    FSlateBrush MakeImageBrush(UTexture2D* Texture, const FLinearColor& Tint)
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(Texture);
        Brush.ImageSize = ButtonImageSize;
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.TintColor = FSlateColor(Tint);
        return Brush;
    }
}

UReadyButtonWidget::UReadyButtonWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 패키징할 때 함께 들어가도록 참조해 둔다
    static ConstructorHelpers::FObjectFinder<UTexture2D> NormalTex(TEXT("/Game/Game/Textures/MainScreenUI/READY_Button.READY_Button"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> HoverTex(TEXT("/Game/Game/Textures/MainScreenUI/READY_Button_Hover.READY_Button_Hover"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> PushTex(TEXT("/Game/Game/Textures/MainScreenUI/START_Button_Push.START_Button_Push"));
    NormalTexture = NormalTex.Object;
    HoverTexture = HoverTex.Object;
    PushTexture = PushTex.Object;
}

TSharedRef<SWidget> UReadyButtonWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }

    return Super::RebuildWidget();
}

void UReadyButtonWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    ClickSound = LoadObject<USoundBase>(nullptr, ClickSoundPath);
    HoverSound = LoadObject<USoundBase>(nullptr, HoverSoundPath);

    // START 버튼은 안에 내용물이 없어 여백(가로 24, 세로 3)만으로 크기가 정해지고,
    // ScaleBox가 그 8:1 비율로 칸에 맞춘다. 준비 버튼도 같은 8:1 상자에 넣어 똑같은 크기가 되게 한다.
    UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("ReadyScale"));
    Scale->SetStretch(EStretch::ScaleToFit);
    WidgetTree->RootWidget = Scale;

    USizeBox* Ratio = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ReadyRatio"));
    Ratio->SetWidthOverride(LayoutSize.X);
    Ratio->SetHeightOverride(LayoutSize.Y);
    Scale->SetContent(Ratio);

    Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ReadyButton"));
    Button->OnClicked.AddDynamic(this, &UReadyButtonWidget::OnClicked);
    Ratio->SetContent(Button);

    Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReadyLabel"));
    Label->SetJustification(ETextJustify::Center);
    Button->SetContent(Label);

    ApplyState(false);
}

void UReadyButtonWidget::ApplyState(bool bReady)
{
    if (!Button || !Label)
    {
        return;
    }

    // START 버튼과 같은 상태별 그림/색(누를 때는 START처럼 Push 그림)
    // 준비된 상태는 살짝 어둡게 해서 눌려 있는 느낌을 준다
    const float Shade = bReady ? 0.8f : 1.f;
    FButtonStyle Style = Button->GetStyle();
    Style.SetNormal(MakeImageBrush(NormalTexture, FLinearColor(Shade, Shade, Shade, 1.f)));
    Style.SetHovered(MakeImageBrush(HoverTexture, FLinearColor(0.724f, 0.724f, 0.724f, 1.f)));
    Style.SetPressed(MakeImageBrush(PushTexture, FLinearColor::Black));
    Style.SetNormalPadding(FMargin(12.f, 1.5f));
    Style.SetPressedPadding(FMargin(12.f, 1.5f));
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

    FSlateFontInfo Font = BoldFont ? FSlateFontInfo(BoldFont.Get(), LabelFontSize) : FCoreStyle::GetDefaultFontStyle("Bold", LabelFontSize);
    Font.OutlineSettings.OutlineSize = LabelOutlineSize;
    Font.OutlineSettings.OutlineColor = TextOutline;
    Label->SetFont(Font);
    Label->SetColorAndOpacity(FSlateColor(TextFill));
    Label->SetText(FText::FromString(bReady ? TEXT("준비 완료!") : TEXT("준비하기")));
}

void UReadyButtonWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    const APlayerController* PC = GetOwningPlayer();
    const AMyPlayerState* PS = PC ? PC->GetPlayerState<AMyPlayerState>() : nullptr;

    // 방장: START / 그 외: 준비 버튼 (바뀔 때만)
    const int32 Host = PS ? (PS->IsHost() ? 1 : 0) : -1;
    if (Host != LastHost)
    {
        LastHost = Host;
        const bool bShowReady = Host == 0;
        SetVisibility(bShowReady ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
        if (UWidget* Start = StartWidget.Get())
        {
            Start->SetVisibility(Host == 1 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
        }
    }

    const int32 Ready = (PS && PS->IsReady()) ? 1 : 0;
    if (Ready != LastReady)
    {
        LastReady = Ready;
        ApplyState(Ready == 1);
    }
}

void UReadyButtonWidget::OnClicked()
{
    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
    {
        PC->RequestToggleReady();
    }
}
