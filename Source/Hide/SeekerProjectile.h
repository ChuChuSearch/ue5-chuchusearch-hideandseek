#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SeekerProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class APropBase;

UCLASS()
class HIDE_API ASeekerProjectile : public AActor
{
    GENERATED_BODY()

public:
    ASeekerProjectile();

protected:
    UPROPERTY(VisibleAnywhere)
    USphereComponent* Collision = nullptr;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh = nullptr;

    /** Visual-only projectile effect. Collision and movement remain controlled by this actor. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    UNiagaraComponent* ProjectileEffect = nullptr;

    UPROPERTY(VisibleAnywhere)
    UProjectileMovementComponent* ProjectileMovement = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float LifeSeconds = 3.0f;

    /** Effect used when the projectile hits a prop or a non-seeker character. */
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* TargetImpactEffect = nullptr;

    /** Effect used when the projectile hits level geometry or any other actor. */
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* WorldImpactEffect = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Effects", meta = (ClampMin = "0.01"))
    float ImpactEffectScale = 1.0f;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastSpawnImpactEffect(bool bTargetImpact, FVector_NetQuantize ImpactPoint,
        FVector_NetQuantizeNormal ImpactNormal);

    void DestroyHitProp(APropBase* Prop);

    virtual void BeginPlay() override;
};
