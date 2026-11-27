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
class USoundBase;
class UStaticMeshComponent;
class UPointLightComponent;

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
    void UpdateVisualEffects(float DeltaSeconds);

    UPROPERTY(VisibleAnywhere)
    USceneComponent* Root = nullptr;

    UPROPERTY(VisibleAnywhere)
    USphereComponent* OverlapSphere = nullptr;

    UPROPERTY(VisibleAnywhere)
    UWidgetComponent* ClueWidget = nullptr;

    // 멀리서도 위치를 알 수 있게 세우는 빛기둥
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* BeamMesh = nullptr;

    // 주변을 밝혀 어두운 곳에서도 눈에 띄게 하는 빛
    UPROPERTY(VisibleAnywhere)
    UPointLightComponent* GlowLight = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Clue")
    int32 ClueNumber = 0;

    // 아이콘 크기(기존 0.35는 너무 작아 잘 안 보였음)
    UPROPERTY(EditDefaultsOnly, Category = "Clue|Visual", meta = (ClampMin = "0.1"))
    float IconWorldScale = 1.0f;

    // 아이콘이 위아래로 둥실거리는 높이/속도
    UPROPERTY(EditDefaultsOnly, Category = "Clue|Visual")
    float BobHeight = 14.f;

    UPROPERTY(EditDefaultsOnly, Category = "Clue|Visual")
    float BobSpeed = 2.4f;

    // 나타날 때 커지면서 튀어나오는 시간
    UPROPERTY(EditDefaultsOnly, Category = "Clue|Visual", meta = (ClampMin = "0.01"))
    float PopInDuration = 0.35f;

    UPROPERTY(EditDefaultsOnly, Category = "Clue|Visual")
    float BeamHeight = 900.f;

    UPROPERTY(EditDefaultsOnly, Category = "Clue|Visual")
    float BeamWidth = 22.f;

    UPROPERTY(EditDefaultsOnly, Category = "Clue|Visual")
    float GlowIntensity = 6000.f;

    // 새 단서를 얻은 플레이어에게만 들림
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* PickupSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (ClampMin = "0.0"))
    float PickupSoundVolume = 1.0f;

private:
    float VisualAge = 0.f;
    FVector IconBaseLocation = FVector::ZeroVector;
};
