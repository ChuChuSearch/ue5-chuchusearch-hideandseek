#include "MusicSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

USoundBase* UMusicSubsystem::GetCurrentMusic() const
{
    return (IsValid(MusicComponent) && MusicComponent->IsPlaying()) ? CurrentMusic.Get() : nullptr;
}

void UMusicSubsystem::Play(UObject* WorldContext, USoundBase* Music, float Volume, float FadeSeconds, bool bPersistAcrossMaps)
{
    // 같은 곡이 이어서 나오는 중이면 그대로(맵이 바뀌어도 끊기지 않게)
    if (Music && Music == CurrentMusic && IsValid(MusicComponent) && MusicComponent->IsPlaying())
    {
        MusicComponent->SetVolumeMultiplier(Volume);
        return;
    }

    Stop(FadeSeconds);

    if (!Music || !WorldContext)
    {
        return;
    }

    // 맵을 넘어 이어질 곡은 bPersistAcrossLevelTransition=true. 아니면 맵이 바뀔 때 엔진이 정리한다.
    MusicComponent = UGameplayStatics::SpawnSound2D(WorldContext, Music, Volume, 1.f, 0.f, nullptr, bPersistAcrossMaps, true);
    if (MusicComponent && FadeSeconds > 0.f)
    {
        // 음량은 위에서 정했으므로 페이드는 1(그대로)까지만
        MusicComponent->FadeIn(FadeSeconds, 1.f);
    }
    CurrentMusic = Music;
}

void UMusicSubsystem::Stop(float FadeSeconds)
{
    if (IsValid(MusicComponent))
    {
        if (FadeSeconds > 0.f)
        {
            MusicComponent->FadeOut(FadeSeconds, 0.f);
        }
        else
        {
            MusicComponent->Stop();
        }
    }

    MusicComponent = nullptr;
    CurrentMusic = nullptr;
}
