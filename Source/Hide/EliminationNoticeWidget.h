#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EliminationNoticeWidget.generated.h"

class UVerticalBox;
class UWidget;

// 화면 위쪽에 "○○ 님이 잡혔어요!" 알림을 잠깐 띄웠다가 사라지게 한다(모든 플레이어).
// WBP 없이 C++에서 그린다. 여러 명이 연달아 탈락하면 아래로 쌓인다.
UCLASS()
class HIDE_API UEliminationNoticeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void AddNotice(const FString& RunnerName, int32 RemainingRunners);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildTree();
    FSlateFontInfo MakeFont(int32 Size, const FLinearColor& OutlineColor, int32 OutlineSize) const;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> List;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UWidget>> Entries;

    TArray<float> EntryAges;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;
};
