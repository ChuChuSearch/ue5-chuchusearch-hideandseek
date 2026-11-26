#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VolumeSettingsWidget.generated.h"

class AMyPlayerController;
class USlider;
class UTextBlock;
class UButton;

// 볼륨 설정 창. WBP 없이 C++에서 위젯 트리를 직접 만든다.
UCLASS()
class HIDE_API UVolumeSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitFromController(AMyPlayerController* InPC);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    void BuildTree();
    USlider* AddRow(class UVerticalBox* Box, const FText& Label, TObjectPtr<UTextBlock>& OutPercent);
    void RefreshFromController();
    static FText PercentText(float Value);

    UFUNCTION()
    void OnMasterChanged(float Value);

    UFUNCTION()
    void OnMusicChanged(float Value);

    UFUNCTION()
    void OnSFXChanged(float Value);

    UFUNCTION()
    void OnCloseClicked();

    UPROPERTY(Transient)
    TObjectPtr<AMyPlayerController> OwnerPC;

    UPROPERTY(Transient)
    TObjectPtr<USlider> MasterSlider;

    UPROPERTY(Transient)
    TObjectPtr<USlider> MusicSlider;

    UPROPERTY(Transient)
    TObjectPtr<USlider> SFXSlider;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> MasterPercent;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> MusicPercent;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SFXPercent;
};
