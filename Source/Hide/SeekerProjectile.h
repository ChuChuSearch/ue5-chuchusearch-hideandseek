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
class USoundBase;
class USoundAttenuation;

UCLASS()
class HIDE_API ASeekerProjectile : public AActor
{
    GENERATED_BODY()

public:
    ASeekerProjectile();

    UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

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

    // 맞지 않은 숨는 사람이 떨어진 곳에서 이 거리(cm) 안에 있으면 아슬아슬 회피 점수
    UPROPERTY(EditDefaultsOnly, Category = "Score", meta = (ClampMin = "0.0"))
    float NearMissRadius = 200.f;

    void AwardNearMisses(const FVector& ImpactPoint, const class AMyCharacter* HitRunner) const;

    /** Effect used when the projectile hits a prop or a non-seeker character. */
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* TargetImpactEffect = nullptr;

    /** Effect used when the projectile hits level geometry or any other actor. */
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* WorldImpactEffect = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Effects", meta = (ClampMin = "0.01"))
    float ImpactEffectScale = 1.0f;

    // ===== Sound =====
    /** 발사 순간 투사체 위치에서 재생 */
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* FireSound = nullptr;

    /** 러너/프롭에 맞았을 때 */
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* TargetImpactSound = nullptr;

    /** 벽/바닥 등에 맞았을 때 */
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* WorldImpactSound = nullptr;

    /** 거리 감쇠 설정. 비워 두면 사운드 에셋 자체 설정을 따른다. */
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundAttenuation* SoundAttenuation = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (ClampMin = "0.0"))
    float FireSoundVolume = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (ClampMin = "0.0"))
    float ImpactSoundVolume = 1.0f;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastSpawnImpactEffect(bool bTargetImpact, FVector_NetQuantize ImpactPoint,
        FVector_NetQuantizeNormal ImpactNormal);

    void DestroyHitProp(APropBase* Prop);

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
