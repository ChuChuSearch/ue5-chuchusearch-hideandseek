#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LobbyGameSettings.generated.h"

// 방장이 대기실에서 정하는 게임 설정(최대 인원, 단계별 시간).
// 시간은 모두 초 단위.
USTRUCT(BlueprintType)
struct FLobbyGameSettings
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 MaxPlayers = 4;

    UPROPERTY(BlueprintReadOnly)
    int32 HideTimeSeconds = 30;

    UPROPERTY(BlueprintReadOnly)
    int32 Phase1Seconds = 240;

    UPROPERTY(BlueprintReadOnly)
    int32 ForcedSwapSeconds = 30;

    UPROPERTY(BlueprintReadOnly)
    int32 Phase2Seconds = 240;

    UPROPERTY(BlueprintReadOnly)
    int32 FeverTimeSeconds = 60;

    bool operator==(const FLobbyGameSettings& Other) const
    {
        return MaxPlayers == Other.MaxPlayers
            && HideTimeSeconds == Other.HideTimeSeconds
            && Phase1Seconds == Other.Phase1Seconds
            && ForcedSwapSeconds == Other.ForcedSwapSeconds
            && Phase2Seconds == Other.Phase2Seconds
            && FeverTimeSeconds == Other.FeverTimeSeconds;
    }
    bool operator!=(const FLobbyGameSettings& Other) const { return !(*this == Other); }

    int32 GetTotalSeconds() const
    {
        return HideTimeSeconds + Phase1Seconds + ForcedSwapSeconds + Phase2Seconds + FeverTimeSeconds;
    }
};

// 설정 항목(창의 줄 순서와 같다)
enum class ELobbySettingField : uint8
{
    MaxPlayers,
    HideTime,
    Phase1,
    ForcedSwap,
    Phase2,
    FeverTime,
    Count
};

// 항목별 고를 수 있는 값과 표시 방법. 서버 검증과 UI가 같은 목록을 쓴다.
struct HIDE_API FLobbySettingRules
{
    static const TArray<int32>& GetOptions(ELobbySettingField Field);
    static FString GetLabel(ELobbySettingField Field);
    static FString FormatValue(ELobbySettingField Field, int32 Value);

    static int32 GetValue(const FLobbyGameSettings& Settings, ELobbySettingField Field);
    static void SetValue(FLobbyGameSettings& Settings, ELobbySettingField Field, int32 Value);

    // 목록 안의 가장 가까운 값으로 맞춘다(서버에서 잘못된 값 막기)
    static int32 SnapToOption(ELobbySettingField Field, int32 Value);
    static FLobbyGameSettings Sanitize(const FLobbyGameSettings& In);

    // 목록에서 Step(±1)만큼 옮긴 값. 끝이면 그대로.
    static int32 StepValue(ELobbySettingField Field, int32 Current, int32 Step);

    static FString FormatTime(int32 Seconds);
};

// 서버의 GameInstance에 붙어 맵이 바뀌어도(대기실 ↔ 게임) 설정을 기억한다.
UCLASS()
class HIDE_API ULobbySettingsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    bool IsInitialized() const { return bInitialized; }
    const FLobbyGameSettings& GetSettings() const { return Settings; }
    void SetSettings(const FLobbyGameSettings& InSettings);

    // 처음 한 번만(게임 모드 블루프린트의 기본값으로) 채운다
    void InitializeDefaults(const FLobbyGameSettings& Defaults);

    // 블루프린트에 설정된 처음 값(“기본값으로” 버튼용)
    const FLobbyGameSettings& GetDefaults() const { return DefaultSettings; }

private:
    FLobbyGameSettings Settings;
    FLobbyGameSettings DefaultSettings;
    bool bInitialized = false;
};
