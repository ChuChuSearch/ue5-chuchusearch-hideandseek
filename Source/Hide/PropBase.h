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

    void SetPossessedState(bool bPossessed);
    void SetPossessedNet(bool bNew);
    void SetPossessionBanned_Server(bool bNewBanned);

    UStaticMeshComponent* GetStaticMesh() const { return StaticMesh; }
    float GetBottomOffsetFromActorLocation() const;

    UFUNCTION(BlueprintCallable)
    FName GetPropId() const { return PropId; }

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
};
