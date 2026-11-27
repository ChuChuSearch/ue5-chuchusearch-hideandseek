#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpectateHudWidget.generated.h"

class UTextBlock;

// 유령 관전 중 화면 아래 가운데 안내: "관전 중  OOO  술래" + 조작 안내.
UCLASS()
class HIDE_API USpectateHudWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetTarget(const FString& Name, const FString& RoleText, const FLinearColor& RoleColor, int32 Index, int32 Count);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    void BuildTree();

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> NameText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RoleText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CountText;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;
};
