#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyHudWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UTextBlock;
class ALobbyDevice;
class AMyPlayerState;

// 대기실 로비 맵 화면 위 표시(WBP 없이 C++에서 그린다)
// - 장치 표지판: 출발 발판 / 역할 문 / 게임 설정 게시판 위에 제목 + 한 줄 설명
// - 다른 사람 머리 위 이름표(역할 지망 색, 방장은 ★)
// - 화면 아래 가운데 "E  ○○" 상호작용 안내
// - 출발 카운트다운 큰 숫자
UCLASS()
class HIDE_API ULobbyHudWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    struct FWorldLabel
    {
        TObjectPtr<UBorder> Box = nullptr;
        TObjectPtr<UTextBlock> Title = nullptr;
        TObjectPtr<UTextBlock> Sub = nullptr;
    };

    void BuildTree();
    FSlateFontInfo MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor = FLinearColor::Transparent, int32 OutlineSize = 0) const;
    FWorldLabel MakeWorldLabel(bool bWithSub);

    // 월드 위치를 화면에 놓는다. 화면 밖/뒤/너무 멀면 숨긴다. 가까울수록 크게.
    bool PlaceAt(UWidget* Widget, const FVector& WorldLocation, float MaxDistance);

    void UpdateDeviceLabels();
    void UpdateNameTags();
    void UpdatePrompt();
    void UpdateCountdown();

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> Canvas;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> PromptBox;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> PromptKey;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PromptText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CountdownText;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> LightFont;

    // 위젯 트리 안의 위젯은 WidgetTree가 붙잡고 있으므로 여기서는 약한 참조 대신 그대로 들고 있다
    TMap<TWeakObjectPtr<ALobbyDevice>, FWorldLabel> DeviceLabels;
    TMap<TWeakObjectPtr<const AMyPlayerState>, FWorldLabel> NameTags;

    float DeviceScanRemain = 0.f;
};
