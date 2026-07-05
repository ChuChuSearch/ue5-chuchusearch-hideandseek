#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SeekerProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
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

    UPROPERTY(VisibleAnywhere)
    UProjectileMovementComponent* ProjectileMovement = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float LifeSeconds = 3.0f;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    void DestroyHitProp(APropBase* Prop);

    virtual void BeginPlay() override;
};
