#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ControlsGuideWidget.generated.h"

class UBorder;
class UTextBlock;
class UVerticalBox;

enum class EGuideRole : uint8
{
    None,
    Runner,
    Seeker,
    Ghost,
    Lobby   // 대기실 로비 맵에서 돌아다닐 때
};

// 역할별 조작법 안내(화면 왼쪽 아래). 역할이 바뀌면 펼쳐서 보여주고,
// 잠시 뒤 "H 조작법" 작은 버튼 모양으로 접힌다. H로 다시 펼치고 접는다.
// WBP 없이 C++에서 그린다.
UCLASS()
class HIDE_API UControlsGuideWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetRole(EGuideRole InRole);
    EGuideRole GetRole() const { return Role; }
    void ToggleExpanded();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildTree();
    void RebuildRows();
    void SetExpanded(bool bInExpanded);
    void AddRow(const FString& Key, const FString& Desc);
    FSlateFontInfo MakeFont(bool bBold, int32 Size) const;

    EGuideRole Role = EGuideRole::None;
    bool bExpanded = false;
    float AutoCollapseRemain = 0.f;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> Card;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> Chip;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> TitleRibbon;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> Rows;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> Footer;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> LightFont;
};
