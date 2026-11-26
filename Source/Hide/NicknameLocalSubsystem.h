#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "NicknameLocalSubsystem.generated.h"

// 로컬 플레이어가 마지막으로 정한 닉네임을 기억한다.
// 결과 후 대기실로 돌아갈 때(재접속 방식 이동) PlayerState가 새로 만들어져도
// LocalPlayer는 그대로 남으므로, 여기서 꺼내 다시 서버에 보낸다.
// PIE 창마다 LocalPlayer가 따로라 서로 섞이지 않는다.
UCLASS()
class HIDE_API UNicknameLocalSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    const FString& GetNickname() const { return Nickname; }
    void SetNickname(const FString& InNickname) { Nickname = InNickname; }

private:
    FString Nickname;
};
