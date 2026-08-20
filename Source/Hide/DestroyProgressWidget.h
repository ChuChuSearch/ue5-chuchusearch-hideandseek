#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DestroyProgressWidget.generated.h"

class AMyCharacter;
class UImage;
class UMaterialInstanceDynamic;
class UTextBlock;

UCLASS(Blueprintable)
class HIDE_API UDestroyProgressWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Destroy Progress")
    void BeginDestroyProgress(AMyCharacter* InCharacter);

    UFUNCTION(BlueprintCallable, Category = "Destroy Progress")
    void EndDestroyProgress();

    UFUNCTION(BlueprintCallable, Category = "Destroy Progress")
    float GetCurrentProgress() const { return CurrentProgress; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Destroy Progress")
    UImage* ProgressImage = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Destroy Progress")
    UTextBlock* ProgressText = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destroy Progress")
    FName ProgressMaterialParameterName = TEXT("Progress");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destroy Progress")
    bool bHideWhenInactive = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destroy Progress", meta = (ClampMin = "1.0"))
    FVector2D ProgressWidgetSize = FVector2D(96.f, 96.f);

    UFUNCTION(BlueprintImplementableEvent, Category = "Destroy Progress")
    void OnDestroyProgressUpdated(float Progress, float RemainingSeconds);

    UFUNCTION(BlueprintImplementableEvent, Category = "Destroy Progress")
    void OnDestroyProgressActiveChanged(bool bIsActive);

private:
    void UpdateViewportPlacement();
    void RefreshProgress();
    void ApplyProgress(float Progress, float RemainingSeconds);

    UPROPERTY()
    AMyCharacter* SourceCharacter = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* ProgressMaterial = nullptr;

    bool bProgressActive = false;
    float CurrentProgress = 0.f;
};
