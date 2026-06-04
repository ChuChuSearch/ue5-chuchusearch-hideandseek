#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyGameState.h"
#include "GhostCharacter.generated.h"

class AGhostPlacementPreview;
class AMyPlayerController;
class UStaticMesh;
class UInputComponent;

UCLASS()
class HIDE_API AGhostCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AGhostCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

    UFUNCTION(BlueprintCallable)
    bool IsPlacementMode() const { return bPlacementMode; }

    UFUNCTION(BlueprintCallable)
    void TogglePlacementMode();

    UFUNCTION(BlueprintCallable)
    void CancelPlacementMode();

    UFUNCTION(BlueprintCallable)
    void SetSelectedRespawnIndex(int32 InIndex);

    UFUNCTION(BlueprintCallable)
    int32 GetSelectedRespawnIndex() const { return SelectedRespawnIndex; }

    UFUNCTION(BlueprintCallable)
    void TryConfirmPlacement();

    UFUNCTION(BlueprintCallable)
    void TryRefreshRespawnList();

    UFUNCTION(BlueprintCallable)
    float GetPlacementCooldownRemaining() const;

    UFUNCTION(BlueprintCallable)
    float GetPlacementCooldownDuration() const { return PlacementCooldownSeconds; }

    void RefreshLocalGhostVisibility();
    void ClearPlacementSelection();
protected:
    bool GetPlacementHit(FHitResult& OutHit) const;
    void UpdatePlacementPreview();
    void RebuildPreviewFromSelection();
    void DestroyPreview();

    bool IsPlacementValid_Local(const FVector& Location, const FRotator& Rotation) const;
    bool IsNearAnyLivePlayer(const FVector& Location, float Radius) const;
    void UpdatePlacementRotation(float DeltaSeconds);

    UFUNCTION(Server, Reliable)
    void ServerRequestPlaceRespawnProp(int32 RespawnIndex, FVector_NetQuantize Location, FRotator Rotation);

    UFUNCTION(Server, Reliable)
    void ServerRequestRefreshRespawnList();

    void ApplyPlacementModeInput(bool bEnable);
    void StartLocalCooldownPrediction();

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Ghost|Placement")
    TSubclassOf<AGhostPlacementPreview> PlacementPreviewClass = nullptr;

    UPROPERTY()
    AGhostPlacementPreview* PlacementPreviewActor = nullptr;

    UPROPERTY()
    UStaticMesh* SelectedPreviewMesh = nullptr;

    UPROPERTY()
    FVector SelectedPreviewScale = FVector::OneVector;

    UPROPERTY()
    TSubclassOf<AActor> SelectedPropClass = nullptr;

    UPROPERTY(Replicated)
    bool bPlacementMode = false;

    UPROPERTY(Replicated)
    int32 PlacedCount = 0;

    UPROPERTY()
    int32 SelectedRespawnIndex = INDEX_NONE;

    UPROPERTY()
    float PlacementYaw = 0.f;

    UPROPERTY(EditDefaultsOnly, Replicated, Category = "Ghost|Placement")
    float PlacementCooldownSeconds = 10.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Ghost|Placement")
    int32 MaxPlacedObjects = 0; // 0 이하 = 무제한

    UPROPERTY(EditDefaultsOnly, Category = "Ghost|Placement")
    float PlayerBlockRadius = 150.f;

    UPROPERTY(EditDefaultsOnly, Category = "Ghost|Placement")
    float GroundSnapOffset = 2.f;

    UPROPERTY(EditDefaultsOnly, Category = "Ghost|Placement")
    float PreviewGridSnap = 0.f;

    UPROPERTY(EditDefaultsOnly, Category = "Ghost|Placement")
    float PlacementRotationSpeedDegrees = 120.f;

    UPROPERTY(Replicated)
    float LastPlacementServerWorldTime = -1000.f;
};
