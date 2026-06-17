#include "PasswordDoor.h"

#include "Components/StaticMeshComponent.h"

APasswordDoor::APasswordDoor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardComponent"));
    //RootComponent = BillboardComponent;

    CustomSpriteTexture = nullptr;
}

bool APasswordDoor::CanInteractFrom(const AActor* Interactor) const
{
    if (!Interactor)
    {
        return false;
    }

    return FVector::DistSquared(GetActorLocation(), Interactor->GetActorLocation())
        <= FMath::Square(InteractionDistance);
}
