#include "PropIconSet.h"

#include "Engine/Texture2D.h"

UTexture2D* UPropIconSet::FindIcon(const UClass* PropClass) const
{
    // 블루프린트를 상속한 물체도 있을 수 있어 부모 쪽으로 올라가며 찾는다
    for (const UClass* Class = PropClass; Class; Class = Class->GetSuperClass())
    {
        if (const TObjectPtr<UTexture2D>* Found = Icons.Find(TSoftClassPtr<AActor>(const_cast<UClass*>(Class))))
        {
            return Found->Get();
        }
    }
    return nullptr;
}
