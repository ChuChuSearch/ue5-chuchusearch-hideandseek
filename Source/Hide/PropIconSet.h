#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PropIconSet.generated.h"

class UTexture2D;

// 물체(BP_Prop_*) 블루프린트 클래스 → 사진(아이콘) 텍스처 목록.
// PropIconCommandlet이 에디터에서 만들어 /Game/Game/Data/DA_PropIcons 에 저장한다.
// 플레이어 컨트롤러가 이 에셋을 참조하므로 패키징할 때 사진도 함께 들어간다.
UCLASS(BlueprintType)
class HIDE_API UPropIconSet : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icons")
    TMap<TSoftClassPtr<AActor>, TObjectPtr<UTexture2D>> Icons;

    UTexture2D* FindIcon(const UClass* PropClass) const;
};
