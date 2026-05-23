#include "GhostPlacementPreview.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AGhostPlacementPreview::AGhostPlacementPreview()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

    PreviewMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMeshComp"));
    SetRootComponent(PreviewMeshComp);

    PreviewMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PreviewMeshComp->SetGenerateOverlapEvents(false);
    PreviewMeshComp->SetCastShadow(false);
}

void AGhostPlacementPreview::BeginPlay()
{
    Super::BeginPlay();

    if (PreviewMaterial)
    {
        PreviewMID = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
        if (PreviewMID)
        {
            const int32 MatCount = PreviewMeshComp->GetNumMaterials();
            if (MatCount <= 0)
            {
                PreviewMeshComp->SetMaterial(0, PreviewMID);
            }
            else
            {
                for (int32 i = 0; i < MatCount; ++i)
                {
                    PreviewMeshComp->SetMaterial(i, PreviewMID);
                }
            }
        }
    }
}

void AGhostPlacementPreview::SetPreviewMesh(UStaticMesh* InMesh, const FVector& InScale)
{
    if (!PreviewMeshComp || !InMesh) return;
    PreviewMeshComp->SetStaticMesh(InMesh);
    PreviewMeshComp->SetWorldScale3D(InScale);
}

void AGhostPlacementPreview::SetPreviewTransform(const FVector& InLocation, const FRotator& InRotation)
{
    SetActorLocationAndRotation(InLocation, InRotation);
}

void AGhostPlacementPreview::SetPlacementValid(bool bInValid)
{
    if (!PreviewMID) return;

    PreviewMID->SetScalarParameterValue(TEXT("Opacity"), bInValid ? 0.35f : 0.18f);
    PreviewMID->SetVectorParameterValue(
        TEXT("TintColor"),
        bInValid
        ? FLinearColor(0.2f, 1.0f, 0.2f, 1.0f)
        : FLinearColor(1.0f, 0.2f, 0.2f, 1.0f)
    );
}
