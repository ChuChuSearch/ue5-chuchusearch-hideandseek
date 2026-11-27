#include "PropIconLibrary.h"

#include "PropIconCommandlet.h"

int32 UPropIconLibrary::GeneratePropIcons(const FString& Params)
{
#if WITH_EDITOR
    UPropIconCommandlet* Generator = NewObject<UPropIconCommandlet>();
    return Generator->Main(Params);
#else
    return -1;
#endif
}
