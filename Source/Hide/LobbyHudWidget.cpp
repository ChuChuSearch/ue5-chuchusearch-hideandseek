#include "LobbyHudWidget.h"

#include "LobbyDevices.h"
#include "MyPlayerController.h"
#include "MyPlayerState.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Styling/CoreStyle.h"

namespace
{
    const TCHAR* BoldFontPath = TEXT("/Game/Game/Fonts/Maplestory_Bold_Font.Maplestory_Bold_Font");
    const TCHAR* LightFontPath = TEXT("/Game/Game/Fonts/Maplestory_Light_Font.Maplestory_Light_Font");

    FLinearColor Hex(uint8 R, uint8 G, uint8 B, uint8 A = 255)
    {
        return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
    }

    const FLinearColor CardFill = Hex(255, 252, 242, 235);
    const FLinearColor LabelColor = Hex(122, 74, 30);
    const FLinearColor SubLabelColor = Hex(122, 74, 30, 190);
    const FLinearColor SeekerColor = Hex(214, 64, 70);
    const FLinearColor RunnerColor = Hex(56, 150, 96);
    const FLinearColor AnyColor = Hex(122, 74, 30);
    const FLinearColor KeyFill = Hex(255, 255, 255);
    const FLinearColor KeyLine = Hex(236, 152, 42);
    const FLinearColor DisabledText = Hex(122, 74, 30, 150);

    constexpr float DeviceLabelDistance = 4500.f;
    constexpr float NameTagDistance = 3500.f;
    constexpr float NameTagHeight = 115.f;

    FSlateBrush RoundBox(const FLinearColor& Fill, float Radius, const FLinearColor& Line = FLinearColor::Transparent, float LineWidth = 0.f)
    {
        FSlateRoundedBoxBrush Brush(Fill, Line, LineWidth);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Brush;
    }
}

TSharedRef<SWidget> ULobbyHudWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildTree();
    }
    return Super::RebuildWidget();
}

FSlateFontInfo ULobbyHudWidget::MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor, int32 OutlineSize) const
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

void ULobbyHudWidget::BuildTree()
{
    BoldFont = LoadObject<UObject>(nullptr, BoldFontPath);
    LightFont = LoadObject<UObject>(nullptr, LightFontPath);

    Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
    WidgetTree->RootWidget = Canvas;
    SetVisibility(ESlateVisibility::HitTestInvisible);

    // ----- 화면 아래 가운데: [E] 안내 -----
    PromptBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PromptBox"));
    PromptBox->SetBrush(RoundBox(CardFill, 18.f, KeyLine, 3.f));
    PromptBox->SetPadding(FMargin(10.f, 8.f, 20.f, 9.f));
    UHorizontalBox* PromptLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    PromptBox->SetContent(PromptLine);

    PromptKey = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PromptKey"));
    PromptKey->SetBrush(RoundBox(KeyFill, 8.f, KeyLine, 2.f));
    PromptKey->SetPadding(FMargin(11.f, 2.f, 11.f, 4.f));
    UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    KeyText->SetText(FText::FromString(TEXT("E")));
    KeyText->SetFont(MakeFont(true, 20));
    KeyText->SetColorAndOpacity(FSlateColor(LabelColor));
    PromptKey->SetContent(KeyText);
    if (UHorizontalBoxSlot* S = PromptLine->AddChildToHorizontalBox(PromptKey))
    {
        S->SetVerticalAlignment(VAlign_Center);
    }

    PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PromptText"));
    PromptText->SetFont(MakeFont(true, 20));
    PromptText->SetColorAndOpacity(FSlateColor(LabelColor));
    if (UHorizontalBoxSlot* S = PromptLine->AddChildToHorizontalBox(PromptText))
    {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
    }

    if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(PromptBox))
    {
        S->SetAnchors(FAnchors(0.5f, 1.f));
        S->SetAlignment(FVector2D(0.5, 1.0));
        S->SetPosition(FVector2D(0.0, -150.0));
        S->SetAutoSize(true);
        S->SetZOrder(20);
    }
    PromptBox->SetVisibility(ESlateVisibility::Collapsed);

    // ----- 출발 카운트다운 -----
    CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountdownText"));
    CountdownText->SetFont(MakeFont(true, 64, Hex(150, 70, 10), 4));
    CountdownText->SetColorAndOpacity(FSlateColor(Hex(255, 214, 96)));
    CountdownText->SetJustification(ETextJustify::Center);
    if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(CountdownText))
    {
        S->SetAnchors(FAnchors(0.5f, 0.22f));
        S->SetAlignment(FVector2D(0.5, 0.5));
        S->SetAutoSize(true);
        S->SetZOrder(30);
    }
    CountdownText->SetVisibility(ESlateVisibility::Collapsed);
}

ULobbyHudWidget::FWorldLabel ULobbyHudWidget::MakeWorldLabel(bool bWithSub)
{
    FWorldLabel Label;
    Label.Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Label.Box->SetPadding(bWithSub ? FMargin(16.f, 6.f, 16.f, 8.f) : FMargin(10.f, 3.f, 10.f, 5.f));
    Label.Box->SetHorizontalAlignment(HAlign_Center);

    UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Label.Box->SetContent(Lines);

    Label.Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label.Title->SetJustification(ETextJustify::Center);
    if (UVerticalBoxSlot* S = Lines->AddChildToVerticalBox(Label.Title))
    {
        S->SetHorizontalAlignment(HAlign_Center);
    }

    if (bWithSub)
    {
        Label.Sub = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Label.Sub->SetJustification(ETextJustify::Center);
        Label.Sub->SetFont(MakeFont(false, 14));
        Label.Sub->SetColorAndOpacity(FSlateColor(SubLabelColor));
        if (UVerticalBoxSlot* S = Lines->AddChildToVerticalBox(Label.Sub))
        {
            S->SetHorizontalAlignment(HAlign_Center);
            S->SetPadding(FMargin(0.f, 1.f, 0.f, 0.f));
        }
    }

    if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Label.Box))
    {
        S->SetAlignment(FVector2D(0.5, 1.0));
        S->SetAutoSize(true);
    }
    return Label;
}

bool ULobbyHudWidget::PlaceAt(UWidget* Widget, const FVector& WorldLocation, float MaxDistance)
{
    APlayerController* PC = GetOwningPlayer();
    UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
    if (!PC || !CanvasSlot || !PC->PlayerCameraManager)
    {
        return false;
    }

    const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
    const FVector CamDir = PC->PlayerCameraManager->GetCameraRotation().Vector();
    const float Dist = FVector::Dist(CamLoc, WorldLocation);
    FVector2D ScreenPos;
    if (Dist > MaxDistance
        || FVector::DotProduct(WorldLocation - CamLoc, CamDir) <= 0.f
        || !UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, WorldLocation, ScreenPos, false))
    {
        Widget->SetVisibility(ESlateVisibility::Collapsed);
        return false;
    }

    CanvasSlot->SetPosition(ScreenPos);
    // 가까우면 원래 크기, 멀어지면 조금씩 작게
    const float Scale = FMath::Clamp(1600.f / FMath::Max(Dist, 1.f), 0.65f, 1.f);
    Widget->SetRenderScale(FVector2D(Scale, Scale));
    Widget->SetRenderTransformPivot(FVector2D(0.5f, 1.f));
    Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
    return true;
}

void ULobbyHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!Canvas)
    {
        return;
    }

    // 장치 목록은 가끔만 다시 찾는다
    DeviceScanRemain -= InDeltaTime;
    if (DeviceScanRemain <= 0.f)
    {
        DeviceScanRemain = 1.f;
        if (UWorld* World = GetWorld())
        {
            for (TActorIterator<ALobbyDevice> It(World); It; ++It)
            {
                if (!DeviceLabels.Contains(*It))
                {
                    DeviceLabels.Add(*It, MakeWorldLabel(true));
                }
            }
        }
    }

    UpdateDeviceLabels();
    UpdateNameTags();
    UpdatePrompt();
    UpdateCountdown();
}

void ULobbyHudWidget::UpdateDeviceLabels()
{
    const AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer());
    for (auto It = DeviceLabels.CreateIterator(); It; ++It)
    {
        const ALobbyDevice* Device = It.Key().Get();
        FWorldLabel& Label = It.Value();
        if (!Device)
        {
            if (Label.Box)
            {
                Label.Box->RemoveFromParent();
            }
            It.RemoveCurrent();
            continue;
        }

        if (!PlaceAt(Label.Box, Device->GetLabelLocation(), DeviceLabelDistance))
        {
            continue;
        }

        const FLinearColor Color = Device->GetLabelColor();
        Label.Box->SetBrush(RoundBox(CardFill, 16.f, Color, 3.f));
        Label.Title->SetText(FText::FromString(Device->GetLabelTitle()));
        Label.Title->SetFont(MakeFont(true, 22, FLinearColor::White, 2));
        Label.Title->SetColorAndOpacity(FSlateColor(Color));
        if (Label.Sub)
        {
            const FString Sub = Device->GetLabelSub(PC);
            Label.Sub->SetText(FText::FromString(Sub));
            Label.Sub->SetVisibility(Sub.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        }
    }
}

void ULobbyHudWidget::UpdateNameTags()
{
    const APlayerController* PC = GetOwningPlayer();
    const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    const APlayerState* LocalPS = PC ? PC->PlayerState.Get() : nullptr;

    TSet<const AMyPlayerState*> Seen;
    if (GS)
    {
        for (const APlayerState* Raw : GS->PlayerArray)
        {
            const AMyPlayerState* PS = Cast<AMyPlayerState>(Raw);
            const APawn* Pawn = PS ? PS->GetPawn() : nullptr;
            if (!PS || PS == LocalPS || !Pawn)
            {
                continue;
            }
            Seen.Add(PS);

            FWorldLabel* Tag = NameTags.Find(PS);
            if (!Tag)
            {
                Tag = &NameTags.Add(PS, MakeWorldLabel(false));
            }

            if (!PlaceAt(Tag->Box, Pawn->GetActorLocation() + FVector(0.f, 0.f, NameTagHeight), NameTagDistance))
            {
                continue;
            }

            FLinearColor Color = AnyColor;
            switch (PS->GetPreference())
            {
            case ERolePreference::SeekerWish: Color = SeekerColor; break;
            case ERolePreference::RunnerWish: Color = RunnerColor; break;
            default: break;
            }

            FString Name = PS->GetNickname().IsEmpty() ? TEXT("…") : PS->GetNickname();
            // 글꼴에 없는 기호 대신 글자로
            if (PS->IsHost())
            {
                Name = FString(TEXT("방장 · ")) + Name;
            }
            else if (PS->IsReady())
            {
                Name += TEXT(" · 준비");
            }

            Tag->Box->SetBrush(RoundBox(Color, 12.f, FLinearColor::White, 2.f));
            Tag->Title->SetText(FText::FromString(Name));
            Tag->Title->SetFont(MakeFont(true, 16));
            Tag->Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        }
    }

    // 나간 사람 이름표 정리
    for (auto It = NameTags.CreateIterator(); It; ++It)
    {
        const AMyPlayerState* PS = It.Key().Get();
        if (!PS || !Seen.Contains(PS))
        {
            if (It.Value().Box)
            {
                It.Value().Box->SetVisibility(ESlateVisibility::Collapsed);
            }
            if (!PS)
            {
                if (It.Value().Box)
                {
                    It.Value().Box->RemoveFromParent();
                }
                It.RemoveCurrent();
            }
        }
    }
}

void ULobbyHudWidget::UpdatePrompt()
{
    const AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer());
    const ALobbyDevice* Device = PC ? PC->GetLobbyFocusDevice() : nullptr;

    FString Text;
    bool bEnabled = true;
    if (!Device || !Device->GetPrompt(PC, Text, bEnabled) || Text.IsEmpty())
    {
        PromptBox->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    PromptText->SetText(FText::FromString(Text));
    PromptText->SetColorAndOpacity(FSlateColor(bEnabled ? LabelColor : DisabledText));
    // 지금 누를 수 없는 안내(기다리는 중 등)는 E 칸을 숨긴다
    PromptKey->SetVisibility(bEnabled ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    PromptBox->SetBrush(RoundBox(CardFill, 18.f, bEnabled ? KeyLine : Hex(200, 180, 150), 3.f));
    PromptBox->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULobbyHudWidget::UpdateCountdown()
{
    float Remain = -1.f;
    for (const auto& Pair : DeviceLabels)
    {
        if (const ALobbyStartPad* Pad = Cast<ALobbyStartPad>(Pair.Key.Get()))
        {
            Remain = FMath::Max(Remain, Pad->GetCountdownRemaining());
        }
    }

    if (Remain < 0.f)
    {
        CountdownText->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    const int32 Secs = FMath::CeilToInt(Remain);
    CountdownText->SetText(FText::FromString(Secs > 0 ? FString::Printf(TEXT("출발까지 %d"), Secs) : TEXT("출발!")));
    CountdownText->SetVisibility(ESlateVisibility::HitTestInvisible);
}
