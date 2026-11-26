#include "HideCameraShakes.h"

void UHideShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
    OutInfo.Duration = FCameraShakeDuration(Duration);
}

void UHideShakePattern::StartShakePatternImpl(const FCameraShakePatternStartParams& Params)
{
    Elapsed = 0.f;
    Seed = FMath::FRandRange(0.f, 1000.f);
}

void UHideShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
    Elapsed += Params.DeltaTime;

    const float Remain = 1.f - FMath::Clamp(Elapsed / FMath::Max(Duration, KINDA_SMALL_NUMBER), 0.f, 1.f);
    const float Decay = Remain * Remain; // 처음에 세게, 빠르게 잦아듦
    const float T = Seed + Elapsed * Frequency;

    // 축마다 다른 위상의 노이즈(-1~1)
    const float N1 = FMath::PerlinNoise1D(T);
    const float N2 = FMath::PerlinNoise1D(T + 37.1f);
    const float N3 = FMath::PerlinNoise1D(T + 71.7f);

    OutResult.Rotation = FRotator(N1, N2, N3 * 0.5f) * (RotationAmplitude * Decay * 2.f);
    OutResult.Location = FVector(0.f, N2, N1) * (LocationAmplitude * Decay * 2.f);
}

bool UHideShakePattern::IsFinishedImpl() const
{
    return Elapsed >= Duration;
}

namespace
{
    UHideShakePattern* MakePattern(UCameraShakeBase* Owner, float Duration, float RotAmp, float LocAmp, float Freq)
    {
        UHideShakePattern* Pattern = Owner->CreateDefaultSubobject<UHideShakePattern>(TEXT("Pattern"));
        Pattern->Duration = Duration;
        Pattern->RotationAmplitude = RotAmp;
        Pattern->LocationAmplitude = LocAmp;
        Pattern->Frequency = Freq;
        return Pattern;
    }
}

UHitCameraShake::UHitCameraShake(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bSingleInstance = true;
    SetRootShakePattern(MakePattern(this, 0.35f, 1.6f, 3.f, 22.f));
}

UEliminatedCameraShake::UEliminatedCameraShake(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bSingleInstance = true;
    SetRootShakePattern(MakePattern(this, 0.7f, 3.f, 6.f, 18.f));
}

USeekerImpactCameraShake::USeekerImpactCameraShake(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bSingleInstance = true;
    SetRootShakePattern(MakePattern(this, 0.15f, 0.6f, 1.f, 28.f));
}
