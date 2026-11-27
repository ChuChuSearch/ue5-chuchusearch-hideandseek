#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScoreboardWidget.generated.h"

class AMyPlayerState;
struct FHideScoreStats;
class UBorder;
class UTextBlock;
class UVerticalBox;

// Tab을 누르고 있는 동안 보이는 점수판 + 오른쪽 "점수 얻는 법" 도움말.
// 점수는 판마다 새로 시작한다(AMyPlayerState::ScoreStats).
// WBP 없이 C++에서 그린다. 설정 창과 같은 크림색 패널 + 리본 제목 톤.
UCLASS()
class HIDE_API UScoreboardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 보일 때 바로 최신 내용으로 다시 그린다
    void RefreshNow();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    enum class EHelpGroup : uint8
    {
        Seeker,
        Runner,
        Ghost,
        Common
    };

    struct FHelpLine
    {
        FString Action;
        FString Points;
        FString Detail;   // 비어 있으면 생략
    };

    void BuildTree();
    UBorder* MakePanel(const FString& Title, UVerticalBox*& OutBody, float MinWidth);

    void RebuildRows();
    void AddTeamHeader(const FString& Title, const FLinearColor& Color, int32 Count);
    void AddColumnHeader();
    // 한 줄에서 내용이 바뀌는 글자들(위젯은 그대로 두고 글자만 바꿔야 깜빡이지 않는다)
    struct FRowWidgets
    {
        UTextBlock* Name = nullptr;
        UTextBlock* Status = nullptr;
        UTextBlock* Record = nullptr;
        UTextBlock* Score = nullptr;
    };

    FRowWidgets AddPlayerRow(bool bIsLocal, int32 Rank);
    void FillRow(const FRowWidgets& Row, const AMyPlayerState* PS, bool bIsLocal) const;
    void AddEmptyRow(const FString& Message);

    void RebuildHelp();
    void AddHelpGroup(EHelpGroup Group, const FString& Title, const FLinearColor& Color, const TArray<FHelpLine>& Lines);

    EHelpGroup GetLocalHelpGroup() const;
    FString MakeRecordText(const AMyPlayerState* PS, const FHideScoreStats& Stats) const;

    // 점수판에 보여도 되는 점수(상대 팀은 게임 중 가려짐)
    bool GetVisibleStats(const AMyPlayerState* PS, FHideScoreStats& OutStats) const;
    FString MakeStatusText(const AMyPlayerState* PS, FLinearColor& OutColor) const;

    FSlateFontInfo MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor = FLinearColor::Transparent, int32 OutlineSize = 0) const;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> Rows;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> HelpBody;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> LightFont;

    float RefreshRemain = 0.f;

    // 지금 그려진 줄 순서(술래들, 구분용 nullptr, 숨는 사람들). 같으면 줄을 다시 만들지 않고 글자만 바꾼다.
    TArray<const AMyPlayerState*> LastRowOrder;
    TArray<FRowWidgets> RowWidgets;
    bool bRowsBuilt = false;
    bool bLastAnyHidden = false;

    // 도움말은 내 역할이 바뀔 때만 다시 그린다
    int32 LastHelpGroup = -1;
};
