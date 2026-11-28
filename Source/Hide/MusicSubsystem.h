#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MusicSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

// 배경음 한 곡을 맡아 튼다. GameInstance에 붙어 있어서 맵이 바뀌어도(처음 화면 → 로비 등) 남아 있다.
// 플레이어 컨트롤러는 맵마다 새로 만들어지므로, 지금 어떤 곡이 나오는지는 여기서 기억한다.
UCLASS()
class HIDE_API UMusicSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // 같은 곡이 이미 나오고 있으면 끊지 않고 음량만 맞춘다.
    // bPersistAcrossMaps면 맵이 바뀌어도 이어서 재생한다(처음 화면 / 로비 배경음).
    void Play(UObject* WorldContext, USoundBase* Music, float Volume, float FadeSeconds, bool bPersistAcrossMaps);

    void Stop(float FadeSeconds);

    USoundBase* GetCurrentMusic() const;

private:
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> MusicComponent;

    UPROPERTY(Transient)
    TObjectPtr<USoundBase> CurrentMusic;
};
