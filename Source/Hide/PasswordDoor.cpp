#include "PasswordDoor.h"

#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"

APasswordDoor::APasswordDoor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    SetRootComponent(InteractionBox);

    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardComponent"));
    BillboardComponent->SetupAttachment(InteractionBox);

    CustomSpriteTexture = nullptr;
    ApplyBillboardSettings();
}

void APasswordDoor::BeginPlay()
{
    Super::BeginPlay();

    ApplyBillboardSettings();
}

void APasswordDoor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    ApplyBillboardSettings();
}

void APasswordDoor::ApplyBillboardSettings()
{
    if (InteractionBox)
    {
        InteractionBox->SetBoxExtent(InteractionBoxExtent);
        InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        InteractionBox->SetCollisionObjectType(ECC_WorldDynamic);
        InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
        InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        InteractionBox->SetGenerateOverlapEvents(false);
        InteractionBox->SetHiddenInGame(true);
    }

    if (!BillboardComponent)
    {
        return;
    }

    BillboardComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BillboardComponent->SetGenerateOverlapEvents(false);
    BillboardComponent->SetHiddenInGame(false);
    BillboardComponent->SetVisibility(true);

    if (CustomSpriteTexture)
    {
        BillboardComponent->SetSprite(CustomSpriteTexture);
    }
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
