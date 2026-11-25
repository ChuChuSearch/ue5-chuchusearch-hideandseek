#include "SeekerProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
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

    ProjectileEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileEffect"));
    ProjectileEffect->SetupAttachment(Collision);
    ProjectileEffect->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

        // 반대 방향도 무시: 쏜 캐릭터가 앞으로 걸어가며 자기 투사체에 막히지 않게 한다.
        // (클라에서는 이동 예측 때문에 캐릭터가 복제된 투사체보다 앞서 있어 특히 잘 걸림)
        // BeginPlay는 서버와 각 클라 모두에서 실행되므로 양쪽 이동 판정이 같아진다.
        Inst->MoveIgnoreActorAdd(this);
    }

    SetLifeSpan(LifeSeconds);
}

void ASeekerProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (APawn* Inst = GetInstigator())
    {
        Inst->MoveIgnoreActorRemove(this);
    }

    Super::EndPlay(EndPlayReason);
}

void ASeekerProjectile::OnHit(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent*, FVector, const FHitResult& Hit)
{

    if (!HasAuthority() || !IsValid(OtherActor))
    {
        return;
    }

    if (AMyCharacter* HitChar = Cast<AMyCharacter>(OtherActor))
    {
        const bool bIsRunnerTarget = !HitChar->IsSeeker() && !HitChar->IsEliminated();

        if (bIsRunnerTarget)
        {
            if (APropBase* PossessedProp = HitChar->GetPossessedProp())
            {
                DestroyHitProp(PossessedProp);
            }
            else
            {
                HitChar->ApplySeekerHit_Server();
            }
        }

        MulticastSpawnImpactEffect(bIsRunnerTarget, Hit.ImpactPoint, Hit.ImpactNormal);
        Destroy();
        return;
    }

    if (APropBase* Prop = Cast<APropBase>(OtherActor))
    {
        MulticastSpawnImpactEffect(true, Hit.ImpactPoint, Hit.ImpactNormal);
        DestroyHitProp(Prop);
        Destroy();
        return;
    }

    MulticastSpawnImpactEffect(false, Hit.ImpactPoint, Hit.ImpactNormal);
    Destroy();
}

void ASeekerProjectile::MulticastSpawnImpactEffect_Implementation(
    bool bTargetImpact,
    FVector_NetQuantize ImpactPoint,
    FVector_NetQuantizeNormal ImpactNormal)
{
    UNiagaraSystem* Effect = bTargetImpact ? TargetImpactEffect : WorldImpactEffect;
    if (!Effect || !GetWorld())
    {
        return;
    }

    const FVector SafeNormal = ImpactNormal.IsNearlyZero() ? FVector::UpVector : FVector(ImpactNormal);
    const FRotator EffectRotation = SafeNormal.Rotation();
    const FVector EffectScale(ImpactEffectScale);

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        this,
        Effect,
        ImpactPoint,
        EffectRotation,
        EffectScale,
        true,
        true,
        ENCPoolMethod::AutoRelease,
        true);
}

void ASeekerProjectile::DestroyHitProp(APropBase* Prop)
{
    if (!HasAuthority() || !IsValid(Prop))
    {
        return;
    }

    AMyCharacter* PossessingCharacter = Prop->IsPossessedNet()
        ? Prop->GetClaimingCharacter()
        : nullptr;

    if (IsValid(PossessingCharacter))
    {
        PossessingCharacter->HandlePossessedPropDestroyed_Server(Prop);
    }

    if (AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr)
    {
        GS->RegisterDestroyedProp(Prop);
        GS->TrySpawnClueFromDestroyedProp(Prop);
    }

    Prop->Destroy();

    if (IsValid(PossessingCharacter)
        && !PossessingCharacter->IsSeeker()
        && !PossessingCharacter->IsEliminated())
    {
        PossessingCharacter->ApplySeekerHit_Server();
    }
}
