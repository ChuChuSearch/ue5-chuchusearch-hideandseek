#include "SeekerProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

#include "MyCharacter.h"
#include "PropBase.h"
#include "MyGameState.h"
#include "HideCameraShakes.h"
#include "GameFramework/PlayerController.h"

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

        // 쏜 캐릭터도 자기 투사체를 무시해서 앞으로 걸으며 쏠 때 막히지 않게 한다.
        Inst->MoveIgnoreActorAdd(this);
    }

    // 발사 소리: 투사체는 복제 액터라 BeginPlay가 서버/각 클라에서 실행되므로 RPC 없이 모두에게 들린다.
    // (데디케이티드 서버는 오디오 장치가 없어 자동으로 재생하지 않음)
    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation(), FireSoundVolume, 1.f, 0.f, SoundAttenuation);
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

namespace
{
    // 러너를 맞혔을 때 쏜 술래 화면을 살짝 흔든다(맞혔다는 손맛)
    void ShakeShooterCamera(const AActor* Projectile)
    {
        const APawn* Shooter = Projectile ? Projectile->GetInstigator() : nullptr;
        if (APlayerController* PC = Shooter ? Cast<APlayerController>(Shooter->GetController()) : nullptr)
        {
            PC->ClientStartCameraShake(USeekerImpactCameraShake::StaticClass());
        }
    }
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
            ShakeShooterCamera(this);

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
        // 러너가 빙의 중인 프롭을 맞힌 경우에만(빈 프롭은 흔들지 않음)
        if (const AMyCharacter* Possessor = Prop->GetClaimingCharacter())
        {
            if (!Possessor->IsSeeker() && !Possessor->IsEliminated())
            {
                ShakeShooterCamera(this);
            }
        }

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
    if (!GetWorld())
    {
        return;
    }

    // 충돌 소리(대상/지형 구분). 이펙트가 없어도 소리는 재생한다.
    if (USoundBase* ImpactSound = bTargetImpact ? TargetImpactSound : WorldImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, ImpactPoint, ImpactSoundVolume, 1.f, 0.f, SoundAttenuation);
    }

    UNiagaraSystem* Effect = bTargetImpact ? TargetImpactEffect : WorldImpactEffect;
    if (!Effect)
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
