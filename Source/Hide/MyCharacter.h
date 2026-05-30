#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "MyCharacter.generated.h"

class APropBase;
class ASeekerProjectile;

UCLASS()
class HIDE_API AMyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AMyCharacter();

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void RequestPossessProp(APropBase* Prop);
    void RequestReleaseProp();
    void ForceReleasePossessedProp_Server();
    bool ForceReplacePossessedProp_Server(TSubclassOf<APropBase> NewPropClass);

    void ToggleLocked();
    void RequestRotateLocked(float Dir);

    bool IsPossessing() const { return PossessedProp != nullptr; }
    APropBase* GetPossessedProp() const { return PossessedProp; }
    bool IsLocked() const { return bLocked; }
    bool IsForcedPossessLocked() const { return bForcedPossessLock; }

    UPROPERTY(EditDefaultsOnly, Category = "Possess")
    float MaxPossessDistance = 250.f;

    UPROPERTY(EditDefaultsOnly, Category = "Possess")
    float ForcedPossessLockSeconds = 5.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Rotate")
    float LockedRotateDegPerSec = 200.f;

    bool IsSeeker() const { return bIsSeeker; }

    void SetIsSeeker_Server(bool bNewIsSeeker);

    void RequestDestroyPropSkill(APropBase* Prop);

    UPROPERTY(EditDefaultsOnly, Category = "DestroySkill")
    float DestroySkillRange = 250.f;

    UPROPERTY(EditDefaultsOnly, Category = "DestroySkill")
    float DestroyTimeSeconds = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category = "DestroySkill")
    float DestroyCooldownSeconds = 5.0f;

    void RequestSeekerAttack(); // PC에서 호출 (좌클릭)

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    TSubclassOf<ASeekerProjectile> SeekerProjectileClass = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float SeekerAttackCooldownSeconds = 0.4f; // 연타 방지용

    // ===== Elimination (minimal) =====
    UFUNCTION(BlueprintCallable)
    bool IsEliminated() const { return bEliminated; }

    void SetEliminated_Server(bool bNewEliminated);

    void ApplySeekerHit_Server();

    UFUNCTION(BlueprintCallable)
    int32 GetSeekerHitCount() const { return SeekerHitCount; }

    UFUNCTION(BlueprintCallable)
    int32 GetMaxSeekerHitsBeforeElimination() const { return MaxSeekerHitsBeforeElimination; }

    void ApplyEliminatedVisuals();

private:

    UFUNCTION(Server, Reliable)
    void ServerReleaseProp();

    UFUNCTION(Server, Reliable)
    void ServerToggleLocked();

    UFUNCTION(Server, Unreliable)
    void ServerRotateLocked(float Dir);

    UFUNCTION()
    void OnRep_PossessedProp();

    UFUNCTION()
    void OnRep_Locked();

    void ApplyPossessVisuals(bool bPossessingNow);
    void ApplyMoveIgnoreForProp(APropBase* NewProp);
    void ReleasePossessedPropInternal(bool bIgnoreForcedPossessLock);
    void EndForcedPossessLock();

    float GetCapsuleHalfHeight() const;
    FVector PropWorldLocationForCharacter(APropBase* Prop) const;
    FVector PropReleaseWorldLocation(APropBase* Prop) const;
    FVector ProjectReleaseLocationToGround(APropBase* Prop, const FVector& Location) const;
    bool IsReleaseLocationClear(APropBase* Prop, const FVector& Location) const;

    UPROPERTY(ReplicatedUsing = OnRep_PossessedProp)
    APropBase* PossessedProp = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_Locked)
    bool bLocked = false;

    UPROPERTY(Replicated)
    bool bForcedPossessLock = false;

    UPROPERTY()
    APropBase* LastPossessedProp = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_IsSeeker)
    bool bIsSeeker = false;

    UFUNCTION()
    void OnRep_IsSeeker();

    UFUNCTION(Server, Reliable)
    void ServerTryPossess(APropBase* Prop);

    UFUNCTION(Server, Reliable)
    void ServerTryPossessByPropId(FName PropId);

    UFUNCTION(Server, Reliable)
    void ServerRequestPossess(); // Trace해서 Prop 찾은 뒤 ServerTryPossess로 넘김

    void AttachAndSyncPossess(APropBase* Prop);
    void DetachAndSyncRelease(APropBase* Prop);

    // --- Destroy skill RPC/State ---
    UFUNCTION(Server, Reliable)
    void ServerStartDestroyPropSkill(APropBase* Prop);

    void FinishDestroyPropSkill(); // 서버 타이머 콜백
    void EndDestroyCooldown();     // 서버 타이머 콜백

    UPROPERTY(ReplicatedUsing = OnRep_DestroySkillState)
    bool bIsDestroying = false;

    UPROPERTY(Replicated)
    bool bDestroyOnCooldown = false;

    UPROPERTY()
    APropBase* DestroyTargetProp = nullptr;

    FTimerHandle TH_DestroyFinish;
    FTimerHandle TH_DestroyCooldown;

    UFUNCTION()
    void OnRep_DestroySkillState();

    // --- Seeker attack RPC/State ---
    UFUNCTION(Server, Reliable)
    void ServerSeekerAttack();

    void EndSeekerAttackCooldown();

    UPROPERTY(Replicated)
    bool bSeekerAttackOnCooldown = false;

    FTimerHandle TH_SeekerAttackCooldown;
    FTimerHandle TH_ForcedPossessLock;

    // --- Elimination ---
    UPROPERTY(ReplicatedUsing = OnRep_Eliminated)
    bool bEliminated = false;

    UFUNCTION()
    void OnRep_Eliminated();

    // --- Hit by seeker projectile ---
    UPROPERTY(ReplicatedUsing = OnRep_SeekerHitCount)
    int32 SeekerHitCount = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    int32 MaxSeekerHitsBeforeElimination = 2;

    UFUNCTION()
    void OnRep_SeekerHitCount();
};

