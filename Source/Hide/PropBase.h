#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PropBase.generated.h"

class UStaticMeshComponent;
class AMyCharacter;

UCLASS()
class HIDE_API APropBase : public AActor
{
    GENERATED_BODY()

public:
    APropBase();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;

    bool TryClaim(AMyCharacter* ByCharacter);
    void ReleaseClaim(AMyCharacter* ByCharacter);

    bool IsClaimed() const { return ClaimingCharacter != nullptr; }
    bool IsPossessedNet() const { return bPossessedNet; }

    // 캐릭터 모습을 숨겨야 하는 빙의 상태인지(파괴/제거 중인 프롭은 제외)
    bool ShouldHidePossessor() const { return bPossessedNet && !bEndingPlay; }

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    bool IsPossessionBanned() const { return bPossessionBanned; }
    AMyCharacter* GetClaimingCharacter() const { return ClaimingCharacter; }

    void SetPossessedState(bool bPossessed);
    void SetPossessedNet(bool bNew);
    void SetPossessionBanned_Server(bool bNewBanned);
    void EnsurePropId_Server();
    void FinalizeReleaseTransform(const FVector& Location, const FRotator& Rotation);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastFinalizeReleaseTransform(FVector_NetQuantize Location, FRotator Rotation);

    UStaticMeshComponent* GetStaticMesh() const { return StaticMesh; }
    float GetBottomOffsetFromActorLocation() const;

    // 빙의 해제 시 공중에 떠 있으면 물리로 떨어뜨린다(서버 전용). 착지하면 다시 고정된다.
    void StartFallingIfAirborne_Server();
    bool IsFalling() const;

    // 기울어진 회전을 Yaw만 남기고 똑바로 세운 회전값
    FRotator GetUprightRotation() const;

    UFUNCTION(BlueprintCallable)
    FName GetPropId() const { return PropId; }

protected:
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* StaticMesh = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_ClaimingCharacter)
    AMyCharacter* ClaimingCharacter = nullptr;

    UFUNCTION()
    void OnRep_ClaimingCharacter();

    // 붙은 캐릭터가 바뀌면(빙의/해제) 그 캐릭터의 모습 숨김 상태를 다시 계산한다.
    virtual void OnRep_AttachmentReplication() override;

    // 빙의 관련 복제가 도착했을 때 관련 캐릭터들의 모습을 다시 맞춘다(클라 전용).
    void RefreshRelatedCharacterVisuals();

    TWeakObjectPtr<AMyCharacter> LastVisualCharacter;

    bool bEndingPlay = false;

    UPROPERTY(ReplicatedUsing = OnRep_PossessedNet)
    bool bPossessedNet = false;

    UFUNCTION()
    void OnRep_PossessedNet();

    UPROPERTY(Replicated)
    bool bPossessionBanned = false;

    UPROPERTY(Replicated)
    FName PropId;

    // 바닥에서 이 거리(cm) 이상 떠 있으면 해제 시 떨어뜨린다.
    UPROPERTY(EditDefaultsOnly, Category = "Fall")
    float AirborneCheckDistance = 5.f;

    // 떨어지기 시작한 뒤 이 시간이 지나면 강제로 고정한다(굴러다니기 방지).
    UPROPERTY(EditDefaultsOnly, Category = "Fall")
    float MaxFallSeconds = 6.f;

    // 이 속도(cm/s) 이하로 느려지면 착지로 보고 고정한다.
    UPROPERTY(EditDefaultsOnly, Category = "Fall")
    float SettleSpeedThreshold = 5.f;

    // 낙하 중이거나, 착지 후 캐릭터와 겹쳐 있는 동안 캐릭터(Pawn)와의 충돌을 끈다.
    // 클라의 캐릭터 이동 예측도 같은 충돌 설정을 써야 하므로 복제한다.
    UPROPERTY(ReplicatedUsing = OnRep_PawnCollisionSuppressed)
    bool bPawnCollisionSuppressed = false;

    UFUNCTION()
    void OnRep_PawnCollisionSuppressed();

private:
    bool IsAirborne_Server(float CheckDistance, float* OutGroundDistance = nullptr) const;
    void CheckFallSettled_Server();
    void StopFalling_Server();

    void SetPawnCollisionSuppressed_Server(bool bSuppressed);
    void ApplyPawnCollisionResponse();
    bool IsOverlappingAnyPawn() const;
    void CheckPawnOverlapCleared_Server();

    FTimerHandle TH_FallSettle;
    FTimerHandle TH_PawnOverlap;
    double FallStartTime = 0.0;
};
