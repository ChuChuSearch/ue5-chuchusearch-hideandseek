#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleWidget.generated.h"

class UEditableTextBox;
class UTextBlock;
class UButton;
class USoundBase;

// 처음 화면(ConnectMap). 닉네임과 접속할 주소를 넣고 "방 만들기" 또는 "접속하기".
// 닉네임은 로컬 플레이어에 기억해 두었다가 대기실에 들어가면 바로 서버에 보낸다.
// 닉네임과 마지막 주소는 다음 실행 때도 채워지도록 GameUserSettings에 저장한다.
// WBP 없이 C++에서 그린다.
UCLASS()
class HIDE_API UTitleWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 입력 포커스를 줄 칸(닉네임이 비어 있으면 닉네임, 아니면 주소)
    UWidget* GetInitialFocusWidget() const;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    void BuildTree();
    FSlateFontInfo MakeFont(bool bBold, int32 Size, const FLinearColor& OutlineColor = FLinearColor::Transparent, int32 OutlineSize = 0) const;
    UEditableTextBox* MakeField(const FName Name, const FString& Hint);
    UButton* MakeButton(const FName Name, const FString& Label, const FLinearColor& Fill, const FLinearColor& Hover, const FLinearColor& Line);
    UTextBlock* MakeLabel(const FString& Text);

    // 닉네임을 확인해 로컬에 기억한다. 비어 있으면 안내를 띄우고 false.
    bool CommitNickname();
    void SetStatus(const FString& Text, bool bError);

    UFUNCTION()
    void OnHostHelpClicked();

    void SetHostHelpOpen(bool bOpen);

    UFUNCTION()
    void OnHostClicked();

    UFUNCTION()
    void OnJoinClicked();

    UFUNCTION()
    void OnNicknameCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnAddressCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> NicknameBox;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> AddressBox;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> HostButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> JoinButton;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BoldFont;

    UPROPERTY(Transient)
    TObjectPtr<UObject> LightFont;

    UPROPERTY(Transient)
    TObjectPtr<USoundBase> ClickSound;

    UPROPERTY(Transient)
    TObjectPtr<class UBorder> HostHelpBox;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HostHelpToggleText;

    UPROPERTY(Transient)
    TObjectPtr<UWidget> LogoWidget;

    bool bHostHelpOpen = false;

    bool bLeaving = false;
};
