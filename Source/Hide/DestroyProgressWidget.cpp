#include "DestroyProgressWidget.h"

#include "MyCharacter.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInstanceDynamic.h"

void UDestroyProgressWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ProgressImage)
    {
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
    RefreshProgress();
}

void UDestroyProgressWidget::BeginDestroyProgress(AMyCharacter* InCharacter)
{
    SourceCharacter = InCharacter;
    bProgressActive = IsValid(SourceCharacter) && SourceCharacter->IsDestroyingProp();

    SetVisibility(bProgressActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    OnDestroyProgressActiveChanged(bProgressActive);
    RefreshProgress();
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
