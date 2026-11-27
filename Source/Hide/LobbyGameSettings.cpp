#include "LobbyGameSettings.h"

namespace
{
    // 기본값(블루프린트 값)이 목록에 없으면 목록에 끼워 넣어 그 값도 고를 수 있게 한다
    TArray<int32> GOptions[static_cast<int32>(ELobbySettingField::Count)];

    void EnsureOptions()
    {
        static bool bBuilt = false;
        if (bBuilt)
        {
            return;
        }
        bBuilt = true;

        GOptions[static_cast<int32>(ELobbySettingField::MaxPlayers)] = { 2, 3, 4, 5, 6, 7, 8 };
        GOptions[static_cast<int32>(ELobbySettingField::HideTime)] = { 15, 20, 30, 45, 60, 90 };

        TArray<int32> Phase;
        for (int32 S = 60; S <= 600; S += 30)
        {
            Phase.Add(S);
        }
        GOptions[static_cast<int32>(ELobbySettingField::Phase1)] = Phase;
        GOptions[static_cast<int32>(ELobbySettingField::Phase2)] = Phase;

        GOptions[static_cast<int32>(ELobbySettingField::ForcedSwap)] = { 15, 20, 30, 45, 60 };
        GOptions[static_cast<int32>(ELobbySettingField::FeverTime)] = { 30, 45, 60, 90, 120, 180 };
    }

    TArray<int32>& MutableOptions(ELobbySettingField Field)
    {
        EnsureOptions();
        return GOptions[FMath::Clamp(static_cast<int32>(Field), 0, static_cast<int32>(ELobbySettingField::Count) - 1)];
    }

    void AddOptionIfMissing(ELobbySettingField Field, int32 Value)
    {
        TArray<int32>& Options = MutableOptions(Field);
        if (Value > 0 && !Options.Contains(Value))
        {
            Options.Add(Value);
            Options.Sort();
        }
    }
}

const TArray<int32>& FLobbySettingRules::GetOptions(ELobbySettingField Field)
{
    return MutableOptions(Field);
}

FString FLobbySettingRules::GetLabel(ELobbySettingField Field)
{
    switch (Field)
    {
    case ELobbySettingField::MaxPlayers: return TEXT("최대 인원");
    case ELobbySettingField::HideTime:   return TEXT("숨는 시간");
    case ELobbySettingField::Phase1:     return TEXT("1단계");
    case ELobbySettingField::ForcedSwap: return TEXT("강제 교체");
    case ELobbySettingField::Phase2:     return TEXT("2단계");
    case ELobbySettingField::FeverTime:  return TEXT("피버 타임");
    default:                             return FString();
    }
}

FString FLobbySettingRules::FormatTime(int32 Seconds)
{
    Seconds = FMath::Max(0, Seconds);
    if (Seconds < 60)
    {
        return FString::Printf(TEXT("%d초"), Seconds);
    }
    const int32 Min = Seconds / 60;
    const int32 Sec = Seconds % 60;
    return Sec == 0 ? FString::Printf(TEXT("%d분"), Min) : FString::Printf(TEXT("%d분 %d초"), Min, Sec);
}

FString FLobbySettingRules::FormatValue(ELobbySettingField Field, int32 Value)
{
    if (Field == ELobbySettingField::MaxPlayers)
    {
        return FString::Printf(TEXT("%d명"), Value);
    }
    return FormatTime(Value);
}

int32 FLobbySettingRules::GetValue(const FLobbyGameSettings& Settings, ELobbySettingField Field)
{
    switch (Field)
    {
    case ELobbySettingField::MaxPlayers: return Settings.MaxPlayers;
    case ELobbySettingField::HideTime:   return Settings.HideTimeSeconds;
    case ELobbySettingField::Phase1:     return Settings.Phase1Seconds;
    case ELobbySettingField::ForcedSwap: return Settings.ForcedSwapSeconds;
    case ELobbySettingField::Phase2:     return Settings.Phase2Seconds;
    case ELobbySettingField::FeverTime:  return Settings.FeverTimeSeconds;
    default:                             return 0;
    }
}

void FLobbySettingRules::SetValue(FLobbyGameSettings& Settings, ELobbySettingField Field, int32 Value)
{
    switch (Field)
    {
    case ELobbySettingField::MaxPlayers: Settings.MaxPlayers = Value; break;
    case ELobbySettingField::HideTime:   Settings.HideTimeSeconds = Value; break;
    case ELobbySettingField::Phase1:     Settings.Phase1Seconds = Value; break;
    case ELobbySettingField::ForcedSwap: Settings.ForcedSwapSeconds = Value; break;
    case ELobbySettingField::Phase2:     Settings.Phase2Seconds = Value; break;
    case ELobbySettingField::FeverTime:  Settings.FeverTimeSeconds = Value; break;
    default: break;
    }
}

int32 FLobbySettingRules::SnapToOption(ELobbySettingField Field, int32 Value)
{
    const TArray<int32>& Options = GetOptions(Field);
    if (Options.Num() == 0)
    {
        return Value;
    }

    int32 Best = Options[0];
    for (const int32 Option : Options)
    {
        if (FMath::Abs(Option - Value) < FMath::Abs(Best - Value))
        {
            Best = Option;
        }
    }
    return Best;
}

FLobbyGameSettings FLobbySettingRules::Sanitize(const FLobbyGameSettings& In)
{
    FLobbyGameSettings Out = In;
    for (int32 i = 0; i < static_cast<int32>(ELobbySettingField::Count); ++i)
    {
        const ELobbySettingField Field = static_cast<ELobbySettingField>(i);
        SetValue(Out, Field, SnapToOption(Field, GetValue(In, Field)));
    }
    return Out;
}

int32 FLobbySettingRules::StepValue(ELobbySettingField Field, int32 Current, int32 Step)
{
    const TArray<int32>& Options = GetOptions(Field);
    if (Options.Num() == 0)
    {
        return Current;
    }

    int32 Index = Options.IndexOfByKey(SnapToOption(Field, Current));
    Index = FMath::Clamp(Index + Step, 0, Options.Num() - 1);
    return Options[Index];
}

void ULobbySettingsSubsystem::InitializeDefaults(const FLobbyGameSettings& Defaults)
{
    if (bInitialized)
    {
        return;
    }

    // 블루프린트 값이 목록에 없더라도 고를 수 있게 넣어 둔다
    for (int32 i = 0; i < static_cast<int32>(ELobbySettingField::Count); ++i)
    {
        const ELobbySettingField Field = static_cast<ELobbySettingField>(i);
        AddOptionIfMissing(Field, FLobbySettingRules::GetValue(Defaults, Field));
    }

    DefaultSettings = FLobbySettingRules::Sanitize(Defaults);
    Settings = DefaultSettings;
    bInitialized = true;
}

void ULobbySettingsSubsystem::SetSettings(const FLobbyGameSettings& InSettings)
{
    Settings = FLobbySettingRules::Sanitize(InSettings);
    bInitialized = true;
}
