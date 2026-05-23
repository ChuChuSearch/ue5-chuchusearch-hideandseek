#include "SeekerProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "MyCharacter.h"
#include "PropBase.h"
#include "MyGameState.h"

ASeekerProjectile::ASeekerProjectile()
{
    bReplicates = true;
    SetReplicateMovement(true);

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(12.f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Collision->SetNotifyRigidBodyCollision(true);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Collision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 2000.f;
    ProjectileMovement->MaxSpeed = 2000.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->ProjectileGravityScale = 1.0f;
    ProjectileMovement->bShouldBounce = false;

    Collision->OnComponentHit.AddDynamic(this, &ASeekerProjectile::OnHit);
}

void ASeekerProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* OwnerActor = GetOwner())
    {
        Collision->IgnoreActorWhenMoving(OwnerActor, true);
    }

    if (APawn* Inst = GetInstigator())
    {
        Collision->IgnoreActorWhenMoving(Inst, true);
    }

    SetLifeSpan(LifeSeconds);
}

void ASeekerProjectile::OnHit(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent*, FVector, const FHitResult&)
{

    if (!HasAuthority() || !IsValid(OtherActor))
    {
        return;
    }

    if (AMyCharacter* HitChar = Cast<AMyCharacter>(OtherActor))
    {
        if (!HitChar->IsSeeker() && !HitChar->IsEliminated())
        {
            HitChar->ApplySeekerHit_Server();
        }

        Destroy();
        return;
    }

    if (APropBase* Prop = Cast<APropBase>(OtherActor))
    {

        if (AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr)
        {
            GS->RegisterDestroyedProp(Prop);
            GS->TrySpawnClueFromDestroyedProp(Prop);
        }

        Prop->Destroy();
        Destroy();
        return;
    }

    Destroy();
}
