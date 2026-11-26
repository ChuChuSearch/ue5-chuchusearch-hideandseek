#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HitFlashWidget.generated.h"

// 맞았을 때 화면 가장자리가 빨갛게 번졌다가 사라지는 효과(로컬 전용).
// WBP 없이 C++에서 테두리 여러 겹을 겹쳐 그라데이션처럼 보이게 만든다.
UCLASS()
class HIDE_API UHitFlashWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Intensity: 시작 진하기(0~1), Duration: 사라지는 데 걸리는 시간(초)
    void Play(float Intensity, float Duration);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildTree();

    float StartIntensity = 0.f;
    float FadeDuration = 0.f;
    float Elapsed = 0.f;
    bool bPlaying = false;
};
