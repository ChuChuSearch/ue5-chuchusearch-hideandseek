#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PropIconLibrary.generated.h"

// 에디터 Python에서 물체 사진을 만들 때 부르는 함수.
// (썸네일 렌더링은 커맨드렛에서는 검게 나와서, 일반 에디터를 띄워 이 함수를 부른다)
//
//   UnrealEditor.exe Hide.uproject -ExecutePythonScript="<스크립트>"
//   스크립트: unreal.PropIconLibrary.generate_prop_icons("-Size=256"); unreal.SystemLibrary.quit_editor()
UCLASS()
class HIDE_API UPropIconLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Params: -Size=256, -Only=BP_Prop_001, -SaveRaw. 만든 개수를 돌려준다(에디터가 아니면 -1).
    UFUNCTION(BlueprintCallable, Category = "Prop Icons")
    static int32 GeneratePropIcons(const FString& Params);
};
