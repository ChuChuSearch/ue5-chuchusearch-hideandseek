#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "MyCharacter.generated.h"

class APropBase;
class ASeekerProjectile;
class USoundBase;
class USoundAttenuation;
class UAudioComponent;

UCLASS()
class HIDE_API AMyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AMyCharacter();

    virtual void BeginPlay() override;

    // 물체에 빙의한 동안에는 다른 플레이어가 머리 위(커진 캡슐)에 올라타 실려 가지 않게 한다
    virtual bool CanBeBaseForCharacter(APawn* Pawn) const override;

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void RequestPossessProp(APropBase* Prop);
    void RequestReleaseProp();
    void ForceReleasePossessedProp_Server();
    void HandlePossessedPropDestroyed_Server(APropBase* DestroyedProp);
    bool ForceReplacePossessedProp_Server(TSubclassOf<APropBase> NewPropClass);

    void ToggleLocked();
    void RequestRotateLocked(float Dir);

    // 빙의 중인 프롭이 기울어져 있으면 Yaw만 남기고 똑바로 세운다(R키).
    void RequestResetPropRotation();

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

    UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0.1"))
    float JumpStrengthMultiplier = 1.3f;

    UPROPERTY(EditDefaultsOnly, Category = "Possess", meta = (ClampMin = "0.0"))
    float ReleaseClearanceMargin = 12.f;

    UPROPERTY(EditDefaultsOnly, Category = "Possess", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float PossessedCollisionBoundsScale = 0.9f;

    bool IsSeeker() const { return bIsSeeker; }

    void SetIsSeeker_Server(bool bNewIsSeeker);

    void RequestDestroyPropSkill(APropBase* Prop);
    void RequestCancelDestroyPropSkill();

    UPROPERTY(EditDefaultsOnly, Category = "DestroySkill")
    float DestroySkillRange = 250.f;

    UPROPERTY(EditDefaultsOnly, Category = "DestroySkill")
    float DestroyTimeSeconds = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category = "DestroySkill")
    float DestroyCooldownSeconds = 5.0f;

    // 시간 끌기 방지: 이 숨는 사람 위치에서 소리를 낸다(멀리서도 방향을 알 수 있게 3D로 크게)
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayStallTaunt(USoundBase* Sound);

    // PC에서 호출 (좌클릭). 실제로 발사 요청을 보냈으면 true(조준점 반동 표시용).
    bool RequestSeekerAttack();

    bool IsSeekerAttackOnCooldown() const { return bSeekerAttackOnCooldown; }

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    TSubclassOf<ASeekerProjectile> SeekerProjectileClass = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float SeekerAttackCooldownSeconds = 0.4f; // 연타 방지용

    // true면 숨는 시간(HideTime) 동안 술래의 공격/파괴 스킬을 서버에서 막는다.
    // 기존 플레이 규칙을 유지하기 위해 기본값은 false(허용).
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    bool bBlockSeekerActionsDuringHideTime = false;

    // ===== Elimination (minimal) =====
    UFUNCTION(BlueprintCallable)
    bool IsEliminated() const { return bEliminated; }

    void SetEliminated_Server(bool bNewEliminated);

    // Attacker: 점수를 받을 술래(없으면 점수 없음)
    void ApplySeekerHit_Server(AMyCharacter* Attacker = nullptr);

    // ===== Sound =====
    // 하트가 줄었을 때(맞은 본인만 들림)
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* RunnerHitSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (ClampMin = "0.0"))
    float RunnerHitSoundVolume = 1.0f;

    // 러너가 탈락했을 때
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* RunnerEliminatedSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (ClampMin = "0.0"))
    float RunnerEliminatedSoundVolume = 1.0f;

    // true면 탈락 소리를 모든 플레이어가 듣는다(false면 탈락한 본인만)
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    bool bEliminatedSoundForEveryone = true;

    void PlayRunnerHitSounds_Server(bool bEliminatedNow);

    // 빙의/해제 성공 시(본인만 들림: 술래에게 위치가 드러나지 않게)
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* PossessSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* ReleaseSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (ClampMin = "0.0"))
    float PossessSoundVolume = 1.0f;

    void PlayOwnerSound_Server(USoundBase* Sound, float Volume) const;

    // 술래 파괴 스킬: 진행 중 소리는 술래 위치에서, 완료 소리는 프롭 위치에서 모두에게 3D로 들림
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* DestroyProgressSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* DestroyDoneSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (ClampMin = "0.0"))
    float DestroySoundVolume = 1.0f;

    // 거리 감쇠(비우면 거리와 상관없이 같은 크기)
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundAttenuation* DestroySoundAttenuation = nullptr;

    UFUNCTION(BlueprintCallable)
    int32 GetSeekerHitCount() const { return SeekerHitCount; }

    UFUNCTION(BlueprintCallable)
    int32 GetMaxSeekerHitsBeforeElimination() const { return MaxSeekerHitsBeforeElimination; }

    UFUNCTION(BlueprintCallable, Category = "DestroySkill")
    bool IsDestroyingProp() const { return bIsDestroying; }

    UFUNCTION(BlueprintCallable, Category = "DestroySkill")
    double GetDestroyStartedServerTime() const { return DestroyStartedServerTime; }

    UFUNCTION(BlueprintCallable, Category = "DestroySkill")
    float GetDestroyTimeSeconds() const { return DestroyTimeSeconds; }

    void ApplyEliminatedVisuals();

private:

    UFUNCTION(Server, Reliable)
    void ServerReleaseProp();

    UFUNCTION(Server, Reliable)
    void ServerToggleLocked();

    UFUNCTION(Server, Unreliable)
    void ServerRotateLocked(float Dir);

    UFUNCTION(Server, Reliable)
    void ServerResetPropRotation();

    // 서버 회전량을 실제 경과 시간으로 제한하기 위한 마지막 처리 시각(복제 안 함).
    double LastServerRotateTime = -1.0;

    // 빙의 요청 스팸 방지(서버 전용)
    double LastServerPossessRequestTime = -1.0;
    bool ConsumePossessRequestBudget_Server();

    UPROPERTY(EditDefaultsOnly, Category = "Possess")
    float MinPossessRequestIntervalSeconds = 0.2f;

    // 서버 검증 헬퍼
    bool IsActionBlockedByGamePhase() const;
    bool HasLineOfSightToProp(const APropBase* Prop) const;
    bool IsTeammatePossessedProp(const APropBase* Prop) const;
    void TryPossess_Server(APropBase* Prop, bool bRequireLineOfSight);

    UFUNCTION()
    void OnRep_PossessedProp();

    UFUNCTION()
    void OnRep_Locked();

    void ApplyPossessVisuals(bool bPossessingNow);
    void ApplyMoveIgnoreForProp(APropBase* NewProp);
    void ApplyPossessionCollision(APropBase* NewProp);
    void ReleasePossessedPropInternal(bool bIgnoreForcedPossessLock);
    void EndForcedPossessLock();

    float GetCapsuleHalfHeight() const;
    FVector PropWorldLocationForCharacter(APropBase* Prop) const;
    bool FindCharacterReleaseLocation(APropBase* Prop, FVector& OutLocation) const;
    FVector ProjectCharacterReleaseLocationToGround(APropBase* Prop, const FVector& Location) const;
    bool IsCharacterReleaseLocationClear(APropBase* Prop, const FVector& Location) const;

    UPROPERTY(ReplicatedUsing = OnRep_PossessedProp)
    APropBase* PossessedProp = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_Locked)
    bool bLocked = false;

    UPROPERTY(Replicated)
    bool bForcedPossessLock = false;

    UPROPERTY()
    APropBase* LastPossessedProp = nullptr;

    float OriginalCapsuleRadius = 0.f;
    float OriginalCapsuleHalfHeight = 0.f;
    bool bOriginalCapsuleCollisionSaved = false;
    ECollisionEnabled::Type OriginalCapsuleCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
    ECollisionResponse OriginalCapsulePawnResponse = ECR_Block;
    ECollisionResponse OriginalCapsuleVisibilityResponse = ECR_Block;
    ECollisionResponse OriginalCapsuleCameraResponse = ECR_Block;

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

    UFUNCTION(Server, Reliable)
    void ServerCancelDestroyPropSkill();

    void FinishDestroyPropSkill(); // 서버 타이머 콜백
    void EndDestroyCooldown();     // 서버 타이머 콜백

    UPROPERTY(ReplicatedUsing = OnRep_DestroySkillState)
    bool bIsDestroying = false;

    UPROPERTY(Replicated)
    bool bDestroyOnCooldown = false;

    UPROPERTY(Replicated)
    double DestroyStartedServerTime = 0.0;

    UPROPERTY()
    APropBase* DestroyTargetProp = nullptr;

    FTimerHandle TH_DestroyFinish;
    FTimerHandle TH_DestroyCooldown;

    UFUNCTION()
    void OnRep_DestroySkillState();

    void UpdateDestroyProgressSound();

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayDestroyDoneSound(FVector_NetQuantize Location);

    UPROPERTY(Transient)
    UAudioComponent* DestroyProgressAudio = nullptr;

    // --- Seeker attack RPC/State ---
    // 클라이언트가 실제로 보고 있던 카메라 위치/방향을 함께 보낸다.
    // (서버가 가진 원격 플레이어 카메라 정보는 조금 늦어서 그대로 쓰면 조준점과 다른 곳으로 나간다)
    UFUNCTION(Server, Reliable)
    void ServerSeekerAttack(FVector_NetQuantize10 ClientViewLoc, FVector_NetQuantizeNormal ClientViewDir);

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

