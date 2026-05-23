#include "PasswordDoor.h"

#include "Components/StaticMeshComponent.h"

APasswordDoor::APasswordDoor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    RootComponent = DoorMesh;
    DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
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
