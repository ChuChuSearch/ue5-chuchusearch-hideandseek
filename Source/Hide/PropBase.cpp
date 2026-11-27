#include "PropBase.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "MyCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "MyPlayerState.h"

namespace
{
    constexpr float DropTickSeconds = 1.f / 60.f;
}

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
        EnsurePropId_Server();
    }
}

bool APropBase::CanBeBaseForCharacter(APawn* Pawn) const
{
    if (bPossessedNet || ClaimingCharacter)
    {
        return false;
    }

    return Super::CanBeBaseForCharacter(Pawn);
}

void APropBase::EnsurePropId_Server()
{
    if (!HasAuthority() || !PropId.IsNone())
    {
        return;
    }

    PropId = FName(*GetName());
    ForceNetUpdate();
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

        // 다른 플레이어와의 충돌은 빙의한 캐릭터의 캡슐이 맡는다.
        // 메시가 Pawn을 막으면 캡슐을 따라 움직이며 상대 캡슐을 파고들어 상대를 밀어낸다.
        StaticMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        StaticMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
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

bool APropBase::SweepDown_Server(float Distance, float& OutMoveDistance) const
{
    OutMoveDistance = Distance;
    if (!StaticMesh || !GetWorld())
    {
        return false;
    }

    // 프롭 바닥면 크기의 얇은 박스를 아래로 쓸어 받침을 찾는다.
    // 캐릭터(Pawn)도 받침으로 본다: 누가 아래에 있으면 그 위에서 멈춰 겹쳐 끼지 않게 한다.
    const FBoxSphereBounds Bounds = StaticMesh->Bounds;
    const float BottomZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
    const FVector HalfExtent(
        FMath::Max(Bounds.BoxExtent.X * 0.9f, 1.f),
        FMath::Max(Bounds.BoxExtent.Y * 0.9f, 1.f),
        1.f);
    const FVector Start(Bounds.Origin.X, Bounds.Origin.Y, BottomZ + 2.f);
    const FVector End = Start - FVector(0.f, 0.f, Distance);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(PropDropSweep), false, this);
    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

    FHitResult Hit;
    if (!GetWorld()->SweepSingleByObjectType(
        Hit, Start, End, FQuat::Identity, ObjectParams, FCollisionShape::MakeBox(HalfExtent), Params))
    {
        return false;
    }

    // 박스 중심이 바닥 높이 + 2에서 출발하므로, 박스 바닥이 닿은 지점까지의 거리만큼 내린다.
    OutMoveDistance = Hit.bStartPenetrating
        ? 0.f
        : FMath::Max(0.f, static_cast<float>(Start.Z - Hit.Location.Z) - 1.f);
    return true;
}

void APropBase::StartDropToGround_Server()
{
    if (!HasAuthority() || bPossessedNet || !GetWorld())
    {
        return;
    }

    float MoveDistance = 0.f;
    const bool bHasSupport = SweepDown_Server(AirborneCheckDistance + 2.f, MoveDistance);
    if (bHasSupport)
    {
        // 바닥에 닿아 있거나 거의 닿아 있으면 떨어뜨리지 않는다.
        return;
    }

    DropVelocity = 0.f;
    DroppedDistance = 0.f;
    GetWorldTimerManager().SetTimer(TH_Drop, this, &APropBase::TickDrop_Server, DropTickSeconds, true);
}

void APropBase::TickDrop_Server()
{
    // 떨어지는 중 다시 빙의(예약)되면 중단
    if (bPossessedNet || ClaimingCharacter || GetAttachParentActor())
    {
        StopDrop_Server();
        return;
    }

    DropVelocity += DropGravity * DropTickSeconds;
    const float Step = DropVelocity * DropTickSeconds;

    float MoveDistance = Step;
    const bool bLanded = SweepDown_Server(Step, MoveDistance);

    if (MoveDistance > 0.f)
    {
        AddActorWorldOffset(FVector(0.f, 0.f, -MoveDistance), false, nullptr, ETeleportType::TeleportPhysics);
        DroppedDistance += MoveDistance;
    }

    if (bLanded || DroppedDistance >= MaxDropDistance)
    {
        StopDrop_Server();
    }
}

void APropBase::StopDrop_Server()
{
    GetWorldTimerManager().ClearTimer(TH_Drop);
    DropVelocity = 0.f;
    ForceNetUpdate();
}

void APropBase::SetPlacedByGhost_Server(APlayerState* InPlacer)
{
    PlacedByGhost = InPlacer;
}

void APropBase::AwardDecoyHit_Server()
{
    if (!HasAuthority() || IsClaimed() || IsPossessedNet())
    {
        return;
    }

    if (AMyPlayerState* Placer = Cast<AMyPlayerState>(PlacedByGhost.Get()))
    {
        Placer->AddScore_Server(EScoreReason::DecoyHit);
    }
}
