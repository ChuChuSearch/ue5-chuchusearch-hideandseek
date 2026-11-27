#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyPlayerState.h"
#include "ResultScoreWidget.generated.h"

class UBorder;
class UTextBlock;
class UVerticalBox;

// 결과 화면 오른쪽에 뜨는 "이번 판 결과" 카드.
// 결과 영상이 잠깐 나온 뒤 미끄러지듯 나타나서 MVP와 팀별 순위, 칭호를 보여 준다.
// 게임이 끝나면 서버가 모두의 점수를 공개하므로(AMyPlayerState::RevealScores_Server) 그 값을 쓴다.
UCLASS()
class HIDE_API UResultScoreWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetWinningRole(EFinalRole InWinningRole) { WinningRole = InWinningRole; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    struct FResultEntry
    {
        const AMyPlayerState* PS = nullptr;
        FHideScoreStats Stats;
        FString Name;
        FString Title;   // 칭호(없으면 빈 값)
        bool bLocal = false;
    };

    void BuildShell();
    bool TryBuildContent();
    void AssignTitles(TArray<FResultEntry>& Entries) const;

    void AddMvp(const FResultEntry& Mvp);
    void AddTeam(const FString& TeamName, const FLinearColor& Color, bool bWon, const TArray<FResultEntry>& Team);
    void AddRow(const FResultEntry& Entry, int32 Rank);
    UWidget* MakeNameLine(const FString& Name, bool bLocal, int32 Size, const FLinearColor& Color);

    FSlateFontInfo MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor = FLinearColor::Transparent, int32 OutlineSize = 0) const;

    EFinalRole WinningRole = EFinalRole::None;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> Card;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> Body;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> LightFont;

    float Elapsed = 0.f;
    float BuiltTime = 0.f;
    bool bContentBuilt = false;
};
