#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostPlacementPreview.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class HIDE_API AGhostPlacementPreview : public AActor
{
    GENERATED_BODY()

public:
    AGhostPlacementPreview();

    virtual void BeginPlay() override;

    void SetPreviewMesh(UStaticMesh* InMesh, const FVector& InScale);
    void SetPreviewTransform(const FVector& InLocation, const FRotator& InRotation);
    void SetPlacementValid(bool bInValid);

protected:
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* PreviewMeshComp = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Preview")
    UMaterialInterface* PreviewMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* PreviewMID = nullptr;
};
