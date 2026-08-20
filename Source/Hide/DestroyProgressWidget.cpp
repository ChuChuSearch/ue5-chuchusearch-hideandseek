#include "DestroyProgressWidget.h"

#include "MyCharacter.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"

void UDestroyProgressWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UpdateViewportPlacement();

    if (ProgressImage)
    {
        ProgressImage->SetDesiredSizeOverride(ProgressWidgetSize);

        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ProgressImage->Slot))
        {
            CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
            CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            CanvasSlot->SetPosition(FVector2D::ZeroVector);
            CanvasSlot->SetSize(ProgressWidgetSize);
            CanvasSlot->SetAutoSize(false);
        }

        ProgressMaterial = ProgressImage->GetDynamicMaterial();
    }

    ApplyProgress(0.f, 0.f);

    if (bHideWhenInactive)
    {
        SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UDestroyProgressWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateViewportPlacement();
    RefreshProgress();
}

void UDestroyProgressWidget::BeginDestroyProgress(AMyCharacter* InCharacter)
{
    SourceCharacter = InCharacter;
    bProgressActive = IsValid(SourceCharacter) && SourceCharacter->IsDestroyingProp();

    UpdateViewportPlacement();
    SetVisibility(bProgressActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    OnDestroyProgressActiveChanged(bProgressActive);
    RefreshProgress();
}

void UDestroyProgressWidget::UpdateViewportPlacement()
{
    FVector2D ViewportSize = FVector2D::ZeroVector;

    if (APlayerController* OwningPlayer = GetOwningPlayer())
    {
        int32 SizeX = 0;
        int32 SizeY = 0;
        OwningPlayer->GetViewportSize(SizeX, SizeY);
        ViewportSize = FVector2D(static_cast<float>(SizeX), static_cast<float>(SizeY));
    }

    if (ViewportSize.IsNearlyZero() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->GetViewportSize(ViewportSize);
    }

    if (ViewportSize.IsNearlyZero())
    {
        return;
    }

    SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
    // ViewportSize is expressed in physical pixels. Let UMG remove the current
    // DPI scale so the widget remains exactly centered at every resolution/UI scale.
    SetPositionInViewport(ViewportSize * 0.5f, true);
    SetDesiredSizeInViewport(ProgressWidgetSize);
}

void UDestroyProgressWidget::EndDestroyProgress()
{
    SourceCharacter = nullptr;
    bProgressActive = false;
    ApplyProgress(0.f, 0.f);
    OnDestroyProgressActiveChanged(false);

    if (bHideWhenInactive)
    {
        SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UDestroyProgressWidget::RefreshProgress()
{
    if (!IsValid(SourceCharacter) || !SourceCharacter->IsDestroyingProp())
    {
        if (bProgressActive)
        {
            EndDestroyProgress();
        }
        return;
    }

    const float Duration = FMath::Max(SourceCharacter->GetDestroyTimeSeconds(), KINDA_SMALL_NUMBER);
    const double ServerTime = GetWorld() && GetWorld()->GetGameState()
        ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds()
        : GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    const float Elapsed = FMath::Max(0.f, static_cast<float>(ServerTime - SourceCharacter->GetDestroyStartedServerTime()));
    const float Progress = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
    const float RemainingSeconds = FMath::Max(0.f, Duration - Elapsed);

    ApplyProgress(Progress, RemainingSeconds);
}

void UDestroyProgressWidget::ApplyProgress(float Progress, float RemainingSeconds)
{
    CurrentProgress = Progress;

    if (ProgressMaterial)
    {
        ProgressMaterial->SetScalarParameterValue(ProgressMaterialParameterName, Progress);
    }

    if (ProgressText)
    {
        const int32 Percent = FMath::RoundToInt(Progress * 100.f);
        ProgressText->SetText(FText::AsNumber(Percent));
    }

    OnDestroyProgressUpdated(Progress, RemainingSeconds);
}
