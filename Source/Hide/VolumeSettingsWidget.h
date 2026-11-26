#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VolumeSettingsWidget.generated.h"

class AMyPlayerController;
class USlider;
class UTextBlock;
class UProgressBar;
class UVerticalBox;

// 볼륨 설정 창. WBP 없이 C++에서 위젯 트리를 직접 만든다.
// 대기실 UI(크림색 패널 + 주황 테두리 + 리본 제목 + 코랄 버튼, 메이플스토리 폰트)와 같은 톤으로 그린다.
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
    struct FVolumeRow
    {
        USlider* Slider = nullptr;
        UProgressBar* Fill = nullptr;
        UTextBlock* Percent = nullptr;
    };

    void BuildTree();
    FVolumeRow AddRow(UVerticalBox* Box, const FText& Label);
    void SetRowValue(int32 Index, float Value);
    void RefreshFromController();
    void HandleChanged(int32 Index, float Value);

    FSlateFontInfo MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor = FLinearColor::Transparent, int32 OutlineSize = 0) const;

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
    TArray<TObjectPtr<USlider>> Sliders;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UProgressBar>> Fills;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> Percents;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> LightFont;
};
