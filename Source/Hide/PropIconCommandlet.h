#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PropIconCommandlet.generated.h"

// 물체(BP_Prop_*) 사진을 찍어 텍스처로 저장하고, 물체 → 사진 목록(DA_PropIcons)을 만든다. 에디터 전용.
//
// 실행(에디터를 닫고):
//   UnrealEditor-Cmd.exe Hide.uproject -run=PropIcon -AllowCommandletRendering -unattended -nosplash
// 옵션: -Size=256 (사진 크기), -Only=BP_Prop_001 (한 개만)
//
// 결과:
//   /Game/Game/Textures/PropIcons/T_PropIcon_<블루프린트 이름>  (배경은 투명)
//   /Game/Game/Data/DA_PropIcons                                (UPropIconSet)
//   Saved/PropIcons/*.png                                         (확인용 사진)
UCLASS()
class HIDE_API UPropIconCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UPropIconCommandlet();

    virtual int32 Main(const FString& Params) override;
};
