#include "VolumeSettingsWidget.h"

#include "MyPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Sound/SoundBase.h"

namespace
{
    // 다른 버튼들과 같은 클릭/호버 소리
    const TCHAR* ClickSoundPath = TEXT("/Game/Game/Sounds/UI/SFX_UI_Click.SFX_UI_Click");
    const TCHAR* HoverSoundPath = TEXT("/Game/Game/Sounds/UI/SFX_UI_Hover.SFX_UI_Hover");
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

void UVolumeSettingsWidget::BuildTree()
{
    SetIsFocusable(true);

    // 화면 전체를 어둡게 덮는 바깥 테두리
    UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
    Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
    Dim->SetHorizontalAlignment(HAlign_Center);
    Dim->SetVerticalAlignment(VAlign_Center);
    WidgetTree->RootWidget = Dim;

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
    Panel->SetBrushColor(FLinearColor(0.04f, 0.04f, 0.06f, 0.95f));
    Panel->SetPadding(FMargin(32.f, 24.f));
    Dim->SetContent(Panel);

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Rows"));
    Panel->SetContent(Box);

    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
    Title->SetText(FText::FromString(TEXT("소리 설정")));
    FSlateFontInfo TitleFont = Title->GetFont();
    TitleFont.Size = 26;
    Title->SetFont(TitleFont);
    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Title))
    {
        S->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
        S->SetHorizontalAlignment(HAlign_Center);
    }

    MasterSlider = AddRow(Box, FText::FromString(TEXT("전체")), MasterPercent);
    MusicSlider = AddRow(Box, FText::FromString(TEXT("배경음")), MusicPercent);
    SFXSlider = AddRow(Box, FText::FromString(TEXT("효과음")), SFXPercent);

    MasterSlider->OnValueChanged.AddDynamic(this, &UVolumeSettingsWidget::OnMasterChanged);
    MusicSlider->OnValueChanged.AddDynamic(this, &UVolumeSettingsWidget::OnMusicChanged);
    SFXSlider->OnValueChanged.AddDynamic(this, &UVolumeSettingsWidget::OnSFXChanged);

    UButton* Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
    FButtonStyle Style = Close->GetStyle();
    if (USoundBase* Click = LoadObject<USoundBase>(nullptr, ClickSoundPath))
    {
        Style.PressedSlateSound.SetResourceObject(Click);
    }
    if (USoundBase* Hover = LoadObject<USoundBase>(nullptr, HoverSoundPath))
    {
        Style.HoveredSlateSound.SetResourceObject(Hover);
    }
    Close->SetStyle(Style);
    Close->OnClicked.AddDynamic(this, &UVolumeSettingsWidget::OnCloseClicked);

    UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseText"));
    CloseText->SetText(FText::FromString(TEXT("닫기")));
    CloseText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    Close->SetContent(CloseText);

    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Close))
    {
        S->SetPadding(FMargin(0.f, 20.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Center);
    }

    UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Hint"));
    Hint->SetText(FText::FromString(TEXT("ESC / F10 으로 열고 닫기")));
    Hint->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.5f)));
    FSlateFontInfo HintFont = Hint->GetFont();
    HintFont.Size = 12;
    Hint->SetFont(HintFont);
    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Hint))
    {
        S->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
        S->SetHorizontalAlignment(HAlign_Center);
    }

    RefreshFromController();
}

USlider* UVolumeSettingsWidget::AddRow(UVerticalBox* Box, const FText& Label, TObjectPtr<UTextBlock>& OutPercent)
{
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LabelBox->SetWidthOverride(110.f);
    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    LabelText->SetText(Label);
    LabelBox->SetContent(LabelText);
    if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(LabelBox))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    USizeBox* SliderBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SliderBox->SetWidthOverride(320.f);
    USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
    Slider->SetMinValue(0.f);
    Slider->SetMaxValue(1.f);
    Slider->SetStepSize(0.05f);
    Slider->SetValue(1.f);
    SliderBox->SetContent(Slider);
    if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(SliderBox))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    USizeBox* PercentBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    PercentBox->SetWidthOverride(70.f);
    OutPercent = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    OutPercent->SetJustification(ETextJustify::Right);
    OutPercent->SetText(PercentText(1.f));
    PercentBox->SetContent(OutPercent);
    if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(PercentBox))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Row))
    {
        S->SetPadding(FMargin(0.f, 6.f));
    }

    return Slider;
}

void UVolumeSettingsWidget::RefreshFromController()
{
    if (!OwnerPC || !MasterSlider)
    {
        return;
    }

    const float Master = OwnerPC->GetVolumeSetting(EVolumeChannel::Master);
    const float Music = OwnerPC->GetVolumeSetting(EVolumeChannel::Music);
    const float SFX = OwnerPC->GetVolumeSetting(EVolumeChannel::SFX);

    MasterSlider->SetValue(Master);
    MusicSlider->SetValue(Music);
    SFXSlider->SetValue(SFX);
    MasterPercent->SetText(PercentText(Master));
    MusicPercent->SetText(PercentText(Music));
    SFXPercent->SetText(PercentText(SFX));
}

FText UVolumeSettingsWidget::PercentText(float Value)
{
    return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f)));
}

void UVolumeSettingsWidget::OnMasterChanged(float Value)
{
    MasterPercent->SetText(PercentText(Value));
    if (OwnerPC) OwnerPC->SetVolumeSetting(EVolumeChannel::Master, Value);
}

void UVolumeSettingsWidget::OnMusicChanged(float Value)
{
    MusicPercent->SetText(PercentText(Value));
    if (OwnerPC) OwnerPC->SetVolumeSetting(EVolumeChannel::Music, Value);
}

void UVolumeSettingsWidget::OnSFXChanged(float Value)
{
    SFXPercent->SetText(PercentText(Value));
    if (OwnerPC) OwnerPC->SetVolumeSetting(EVolumeChannel::SFX, Value);
}

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
