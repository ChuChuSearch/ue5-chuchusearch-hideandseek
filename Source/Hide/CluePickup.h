#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CluePickup.generated.h"

class AMyCharacter;
class AMyPlayerState;
class UPrimitiveComponent;
class UWidgetComponent;
class USphereComponent;
class UTexture2D;

UCLASS()
class HIDE_API ACluePickup : public AActor
{
    GENERATED_BODY()

public:
    ACluePickup();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable)
    void SetClueNumber(int32 InNumber) { ClueNumber = InNumber; }

    UFUNCTION(BlueprintCallable)
    int32 GetClueNumber() const { return ClueNumber; }

    UFUNCTION()
    void OnOverlapBegin(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

protected:
    void FaceToLocalPlayerCamera();

    UPROPERTY(VisibleAnywhere)
    USceneComponent* Root = nullptr;

    UPROPERTY(VisibleAnywhere)
    USphereComponent* OverlapSphere = nullptr;

    UPROPERTY(VisibleAnywhere)
    UWidgetComponent* ClueWidget = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Clue")
    int32 ClueNumber = 0;
};
