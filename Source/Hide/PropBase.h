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

    // 공중에서 빙의 해제되면 바닥까지 수직으로 떨어뜨린다(서버 전용, 물리 미사용).
    // 위치는 기존 ReplicatedMovement로 클라에 전달된다.
    void StartDropToGround_Server();

    UFUNCTION(BlueprintCallable)
    FName GetPropId() const { return PropId; }

    // 유령이 놓은 물체면 놓은 플레이어를 기억한다(서버 전용, 속임수 점수용)
    void SetPlacedByGhost_Server(APlayerState* InPlacer);

    // 술래가 이 물체를 부쉈을 때 호출. 아무도 숨어 있지 않은, 유령이 놓은 물체면 놓은 사람에게 속임수 점수를 준다.
    void AwardDecoyHit_Server();

protected:
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* StaticMesh = nullptr;

    UPROPERTY(Replicated)
    AMyCharacter* ClaimingCharacter = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_PossessedNet)
    bool bPossessedNet = false;

    UFUNCTION()
    void OnRep_PossessedNet();

    UPROPERTY(Replicated)
    bool bPossessionBanned = false;

    UPROPERTY(Replicated)
    FName PropId;

    // 바닥에서 이 거리(cm) 이상 떠 있을 때만 떨어뜨린다.
    UPROPERTY(EditDefaultsOnly, Category = "Fall", meta = (ClampMin = "0.0"))
    float AirborneCheckDistance = 5.f;

    // 낙하 가속도(cm/s^2)
    UPROPERTY(EditDefaultsOnly, Category = "Fall", meta = (ClampMin = "1.0"))
    float DropGravity = 1960.f;

    // 이보다 멀리 떨어지면 멈춘다(맵 밖으로 무한히 떨어지는 것 방지).
    UPROPERTY(EditDefaultsOnly, Category = "Fall", meta = (ClampMin = "0.0"))
    float MaxDropDistance = 5000.f;

private:
    // 아래로 Distance만큼 프롭 바닥면을 쓸어 보고, 막히면 true와 실제 이동 가능 거리를 돌려준다.
    bool SweepDown_Server(float Distance, float& OutMoveDistance) const;
    void TickDrop_Server();
    void StopDrop_Server();

    TWeakObjectPtr<APlayerState> PlacedByGhost;

    FTimerHandle TH_Drop;
    float DropVelocity = 0.f;
    float DroppedDistance = 0.f;
};
