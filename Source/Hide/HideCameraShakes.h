#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "HideCameraShakes.generated.h"

// 짧게 흔들리고 빠르게 잦아드는 간단한 흔들림 패턴(플러그인 없이 엔진 기본 클래스만 사용).
UCLASS()
class HIDE_API UHideShakePattern : public UCameraShakePattern
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Shake")
    float Duration = 0.3f;

    // 최대 회전 흔들림(도)
    UPROPERTY(EditAnywhere, Category = "Shake")
    float RotationAmplitude = 1.5f;

    // 최대 위치 흔들림(cm)
    UPROPERTY(EditAnywhere, Category = "Shake")
    float LocationAmplitude = 2.f;

    // 초당 흔들림 횟수
    UPROPERTY(EditAnywhere, Category = "Shake")
    float Frequency = 22.f;

protected:
    virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
    virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override;
    virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
    virtual bool IsFinishedImpl() const override;

private:
    float Elapsed = 0.f;
    float Seed = 0.f;
};

// 러너가 맞았을 때(본인 화면)
UCLASS()
class HIDE_API UHitCameraShake : public UCameraShakeBase
{
    GENERATED_BODY()

public:
    UHitCameraShake(const FObjectInitializer& ObjectInitializer);
};

// 러너가 탈락했을 때(본인 화면) - 더 크고 길게
UCLASS()
class HIDE_API UEliminatedCameraShake : public UCameraShakeBase
{
    GENERATED_BODY()

public:
    UEliminatedCameraShake(const FObjectInitializer& ObjectInitializer);
};

// 술래가 러너를 맞혔거나 프롭 파괴를 끝냈을 때(술래 화면) - 작고 짧게
UCLASS()
class HIDE_API USeekerImpactCameraShake : public UCameraShakeBase
{
    GENERATED_BODY()

public:
    USeekerImpactCameraShake(const FObjectInitializer& ObjectInitializer);
};
