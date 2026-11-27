#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GenericPlatform/GenericWindow.h"
#include "SettingsWidget.generated.h"

class AMyPlayerController;
class UBorder;
class UButton;
class UCheckBox;
class UComboBoxString;
class USlider;
class UTextBlock;
class UProgressBar;
class UVerticalBox;
class UWidgetSwitcher;
class USettingsWidget;

// 드롭다운 선택을 "몇 번째 설정 줄에서 무엇을 골랐는지"로 바꿔 전달한다.
// (UComboBoxString::OnSelectionChanged에는 어느 드롭다운인지가 없어서 드롭다운마다 하나씩 붙인다)
UCLASS()
class HIDE_API USettingsComboHandler : public UObject
{
    GENERATED_BODY()

public:
    TWeakObjectPtr<USettingsWidget> Owner;
    int32 OptionIndex = 0;

    UFUNCTION()
    void HandleSelection(FString Item, ESelectInfo::Type SelectionType);
};

// 설정 창(ESC / F10). 탭으로 그래픽 / 오디오 설정을 바꿔 본다.
// WBP 없이 C++에서 위젯 트리를 직접 만든다.
// 대기실 UI(크림색 패널 + 주황 테두리 + 리본 제목 + 코랄 버튼, 메이플스토리 폰트)와 같은 톤으로 그린다.
// - 오디오: AMyPlayerController의 볼륨 설정(닫을 때 저장)
// - 그래픽: UGameUserSettings. 바꾸면 바로 보여주고, 10초 안에 "유지"를 누르지 않으면 원래대로 되돌린다.
//   여러 값 중 고르는 항목은 드롭다운, 켬/끔 항목은 체크박스.
UCLASS()
class HIDE_API USettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitFromController(AMyPlayerController* InPC);

    // 드롭다운에서 고른 값 적용
    void SelectOption(int32 OptionIndex, const FString& Item);

    // 창을 닫기 직전에 호출. 확인하지 않은 그래픽 변경은 되돌린다.
    void HandleClosing();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    // 그래픽 탭의 드롭다운 설정 종류
    enum class EOption : uint8
    {
        WindowMode,
        Resolution,
        OverallQuality,
        ShadowQuality,
        TextureQuality,
        AntiAliasing,
        FrameLimit,
        Count
    };

    struct FSliderRow
    {
        USlider* Slider = nullptr;
        UProgressBar* Fill = nullptr;
        UTextBlock* Percent = nullptr;
    };

    // 되돌리기용으로 적용 전 그래픽 설정을 통째로 기억해 둔다
    struct FGraphicsSnapshot
    {
        EWindowMode::Type WindowMode = EWindowMode::Fullscreen;
        FIntPoint Resolution = FIntPoint::ZeroValue;
        float ResolutionScaleNormalized = 1.f;
        int32 ViewDistance = 3;
        int32 Shadow = 3;
        int32 GlobalIllumination = 3;
        int32 Reflection = 3;
        int32 AntiAliasing = 3;
        int32 Texture = 3;
        int32 Effects = 3;
        int32 PostProcess = 3;
        int32 Foliage = 3;
        int32 Shading = 3;
        float FrameLimit = 0.f;
        bool bVSync = false;
    };

    void BuildTree();
    UVerticalBox* MakePage(const TCHAR* Name);
    UButton* MakeTabButton(const FString& Label, UTextBlock*& OutText);
    UButton* MakePillButton(const FLinearColor& Fill, const FLinearColor& Hover, const FLinearColor& Pressed, float Radius);
    UButton* MakeTextButton(const FString& Label, const FLinearColor& Fill, const FLinearColor& Hover, const FLinearColor& Pressed, const FLinearColor& TextOutline, float Width, float Height, int32 FontSize);
    UWidget* MakeRowLabel(const FText& Label);
    FSliderRow AddSliderRow(UVerticalBox* Box, const FText& Label);
    void AddComboRow(UVerticalBox* Box, EOption Option, const FText& Label);
    UCheckBox* AddCheckRow(UVerticalBox* Box, const FText& Label, UTextBlock*& OutStateText);
    void SetCheckRowState(UCheckBox* Check, UTextBlock* StateText, bool bOn);
    void AddSectionTitle(UVerticalBox* Box, const FText& Title, bool bFirst);
    void BuildConfirmBanner(UVerticalBox* Box);

    void SelectTab(int32 TabIndex);
    void SetSliderRowValue(int32 Index, float Value);

    void RefreshAudio();
    void RefreshGraphics();
    void RefreshCombo(EOption Option);

    // 그래픽 변경 흐름: Begin(처음이면 지금 상태 저장) -> 값 변경 -> Apply(화면 반영, 10초 카운트다운 다시 시작)
    void BeginGraphicsChange();
    void ApplyGraphicsPreview();
    void KeepGraphics();
    void RevertGraphics();
    FGraphicsSnapshot CaptureGraphics() const;
    void RestoreGraphics(const FGraphicsSnapshot& Snapshot);
    void UpdateConfirmBanner();

    FSlateFontInfo MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor = FLinearColor::Transparent, int32 OutlineSize = 0) const;

    UFUNCTION()
    UWidget* GenerateComboItem(FString Item);

    UFUNCTION()
    void OnVSyncChanged(bool bIsChecked);

    UFUNCTION()
    void OnMasterChanged(float Value);

    UFUNCTION()
    void OnMusicChanged(float Value);

    UFUNCTION()
    void OnSFXChanged(float Value);

    UFUNCTION()
    void OnRenderScaleChanged(float Value);

    UFUNCTION()
    void OnGraphicsTabClicked();

    UFUNCTION()
    void OnAudioTabClicked();

    UFUNCTION()
    void OnControlsTabClicked();

    UFUNCTION()
    void OnSensitivityChanged(float Value);

    UFUNCTION()
    void OnInvertYChanged(bool bIsChecked);

    void RefreshControls();
    void SetSensitivityRowValue(float Value);

    // 조작 탭: 상하 반전 체크박스
    UPROPERTY(Transient)
    TObjectPtr<UCheckBox> InvertYCheck;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> InvertYStateText;

    UFUNCTION()
    void OnKeepClicked();

    UFUNCTION()
    void OnRevertClicked();

    UFUNCTION()
    void OnCloseClicked();

    UFUNCTION()
    void OnLeaveClicked();

    // 지금 탭의 설정을 기본값으로
    UFUNCTION()
    void OnResetClicked();

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ResetButtonText;

    // 게임 나가기 버튼의 "한 번 더 누르면 나가요" 상태
    void SetLeaveConfirm(bool bConfirm);

    UPROPERTY(Transient)
    TObjectPtr<UButton> LeaveButton;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> LeaveButtonText;

    UPROPERTY(Transient)
    TObjectPtr<class USizeBox> LeaveSize;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HintText;

    bool bLeaveConfirm = false;
    float LeaveConfirmRemain = 0.f;

    UPROPERTY(Transient)
    TObjectPtr<AMyPlayerController> OwnerPC;

    UPROPERTY(Transient)
    TObjectPtr<UWidgetSwitcher> Pages;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UButton>> TabButtons;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> TabTexts;

    // 슬라이더 줄: 0 전체, 1 배경음, 2 효과음, 3 렌더링 배율, 4 마우스 감도
    UPROPERTY(Transient)
    TArray<TObjectPtr<USlider>> Sliders;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UProgressBar>> Fills;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> Percents;

    // 드롭다운(EOption 순서)
    UPROPERTY(Transient)
    TArray<TObjectPtr<UComboBoxString>> OptionCombos;

    UPROPERTY(Transient)
    TArray<TObjectPtr<USettingsComboHandler>> ComboHandlers;

    // 체크박스 줄: 수직 동기화
    UPROPERTY(Transient)
    TObjectPtr<UCheckBox> VSyncCheck;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> VSyncStateText;

    // 변경 확인 띠(유지 / 되돌리기 + 남은 초)
    UPROPERTY(Transient)
    TObjectPtr<UBorder> ConfirmBanner;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ConfirmText;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> LightFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> ClickSound;

    UPROPERTY(Transient)
    TObjectPtr<UObject> HoverSound;

    // 해상도 선택지(모니터가 지원하는 전체 화면 해상도), 드롭다운 항목 순서와 같다
    TArray<FIntPoint> Resolutions;

    FGraphicsSnapshot PendingSnapshot;
    bool bPendingConfirm = false;
    float PendingRemain = 0.f;
    int32 LastShownSecond = -1;

    int32 ActiveTab = 0;
    bool bRefreshing = false;

    // 드롭다운 선택 콜백 안에서 그 드롭다운 목록을 다시 만들지 않도록 다음 틱에 화면을 갱신한다
    bool bGraphicsRefreshQueued = false;

    static int32 LastTab;
};
