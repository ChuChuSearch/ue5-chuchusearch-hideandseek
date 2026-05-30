#include "PropBase.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "MyCharacter.h"

APropBase::APropBase()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);

    bAlwaysRelevant = true;
    NetDormancy = DORM_Never;
    NetUpdateFrequency = 66.f;
    MinNetUpdateFrequency = 33.f;

    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    SetRootComponent(StaticMesh);
    StaticMesh->SetIsReplicated(true);

    StaticMesh->SetSimulatePhysics(false);
    StaticMesh->SetEnableGravity(false);
    StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    StaticMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    StaticMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    StaticMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void APropBase::BeginPlay()
{
    Super::BeginPlay();

    if (StaticMesh)
    {
        StaticMesh->SetRenderCustomDepth(true);
        StaticMesh->SetCustomDepthStencilValue(0);
        StaticMesh->SetSimulatePhysics(false);
        StaticMesh->SetEnableGravity(false);
    }

    if (HasAuthority())
    {
        if (PropId.IsNone())
        {
            PropId = FName(*GetName());
        }
    }
}

void APropBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(APropBase, ClaimingCharacter);
    DOREPLIFETIME(APropBase, bPossessedNet);
    DOREPLIFETIME(APropBase, bPossessionBanned);
    DOREPLIFETIME(APropBase, PropId);
}

bool APropBase::TryClaim(AMyCharacter* ByCharacter)
{
    if (!HasAuthority() || !IsValid(ByCharacter)) return false;
    if (bPossessionBanned) return false;
    if (ClaimingCharacter) return false;

    ClaimingCharacter = ByCharacter;
    ForceNetUpdate();
    return true;
}

void APropBase::ReleaseClaim(AMyCharacter* ByCharacter)
{
    if (!HasAuthority()) return;
    if (ClaimingCharacter != ByCharacter) return;

    ClaimingCharacter = nullptr;
    ForceNetUpdate();
}

void APropBase::SetPossessedNet(bool bNew)
{
    if (!HasAuthority()) return;

    bPossessedNet = bNew;
    OnRep_PossessedNet();
    ForceNetUpdate();
}

void APropBase::SetPossessionBanned_Server(bool bNewBanned)
{
    if (!HasAuthority()) return;

    bPossessionBanned = bNewBanned;
    ForceNetUpdate();
}

void APropBase::OnRep_PossessedNet()
{
    SetPossessedState(bPossessedNet);
}

void APropBase::SetPossessedState(bool bPossessed)
{
    if (!StaticMesh) return;

    TArray<UPrimitiveComponent*> PrimitiveComponents;
    GetComponents<UPrimitiveComponent>(PrimitiveComponents);
    for (UPrimitiveComponent* Prim : PrimitiveComponents)
    {
        if (!Prim || Prim == StaticMesh)
        {
            continue;
        }

        const bool bIsOutlineComponent = Prim->GetName().Contains(TEXT("Outline"));
        if (bIsOutlineComponent)
        {
            Prim->SetVisibility(bPossessed, true);
            Prim->SetHiddenInGame(!bPossessed, true);
        }

        Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (bPossessed)
    {
        StaticMesh->SetEnableGravity(false);
        StaticMesh->SetSimulatePhysics(false);

        // hover 판정은 되어야 하므로 QueryOnly 유지
        StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        StaticMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

        // 마우스 라인트레이스용
        StaticMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

        // 카메라 붐이 오브젝트에 막히지 않게 해야 1인칭처럼 안 변함
        StaticMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

        // 플레이어와는 충돌하지 않음
        StaticMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    }
    else
    {
        StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        StaticMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        StaticMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        StaticMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        StaticMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);

        StaticMesh->SetSimulatePhysics(false);
        StaticMesh->SetEnableGravity(false);
    }
}

void APropBase::FinalizeReleaseTransform(const FVector& Location, const FRotator& Rotation)
{
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
    SetPossessedState(false);

    if (UStaticMeshComponent* MeshComp = GetStaticMesh())
    {
        MeshComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
        MeshComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        MeshComp->SetSimulatePhysics(false);
        MeshComp->SetEnableGravity(false);
    }
}

void APropBase::MulticastFinalizeReleaseTransform_Implementation(FVector_NetQuantize Location, FRotator Rotation)
{
    FinalizeReleaseTransform(Location, Rotation);
}

float APropBase::GetBottomOffsetFromActorLocation() const
{
    if (!StaticMesh) return 0.f;

    const float ActorZ = GetActorLocation().Z;
    const float BottomZ = StaticMesh->Bounds.Origin.Z - StaticMesh->Bounds.BoxExtent.Z;
    return ActorZ - BottomZ;
}
