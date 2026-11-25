#include "PropBase.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"
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
        EnsurePropId_Server();
    }
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
    DOREPLIFETIME(APropBase, bPawnCollisionSuppressed);
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
    if (bNew)
    {
        // 다시 빙의되면 낙하/겹침 처리 중단
        GetWorldTimerManager().ClearTimer(TH_FallSettle);
        GetWorldTimerManager().ClearTimer(TH_PawnOverlap);
        bPawnCollisionSuppressed = false;
    }
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
    RefreshRelatedCharacterVisuals();
}

void APropBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 빙의 중에 파괴되면 서버는 해제 처리 직후 바로 Destroy하므로, 클라에는 bPossessedNet=false나
    // 분리(Detach) 복제가 오지 않고 액터 제거만 도착한다. 여기서 빙의자 모습을 다시 계산해 보이게 한다.
    bEndingPlay = true;

    if (EndPlayReason == EEndPlayReason::Destroyed)
    {
        RefreshRelatedCharacterVisuals();
    }

    Super::EndPlay(EndPlayReason);
}

void APropBase::OnRep_ClaimingCharacter()
{
    RefreshRelatedCharacterVisuals();
}

void APropBase::OnRep_AttachmentReplication()
{
    Super::OnRep_AttachmentReplication();
    RefreshRelatedCharacterVisuals();
}

void APropBase::RefreshRelatedCharacterVisuals()
{
    // 이전에 붙어 있던 캐릭터(해제된 쪽)도 다시 계산해야 모습이 돌아온다.
    AMyCharacter* Previous = LastVisualCharacter.Get();

    AMyCharacter* Current = Cast<AMyCharacter>(GetAttachParentActor());
    if (!Current && bPossessedNet)
    {
        Current = ClaimingCharacter;
    }

    if (Previous && Previous != Current)
    {
        Previous->RefreshPossessVisuals();
    }

    if (Current)
    {
        Current->RefreshPossessVisuals(this);
    }

    LastVisualCharacter = Current;
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

        // Use the mesh for close contact with other players; the owner ignores this actor.
        StaticMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        StaticMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    }
    else
    {
        StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        StaticMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        StaticMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        StaticMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        StaticMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);

        // 낙하/겹침 중이면 캐릭터 충돌은 계속 꺼 둔다.
        ApplyPawnCollisionResponse();

        // 낙하 물리 on/off는 서버가 결정하고 ReplicatedMovement(bRepPhysics)로 클라에 동기화된다.
        // 클라에서 여기서 끄면 서버가 켠 낙하 시뮬레이션과 어긋나므로 서버에서만 처리.
        if (HasAuthority())
        {
            StaticMesh->SetSimulatePhysics(false);
            StaticMesh->SetEnableGravity(false);
        }
    }
}

void APropBase::FinalizeReleaseTransform(const FVector& Location, const FRotator& Rotation)
{
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
    SetPossessedState(false);

    if (UStaticMeshComponent* MeshComp = GetStaticMesh(); MeshComp && HasAuthority())
    {
        MeshComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
        MeshComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        MeshComp->SetSimulatePhysics(false);
        MeshComp->SetEnableGravity(false);
    }
}

FRotator APropBase::GetUprightRotation() const
{
    return FRotator(0.f, GetActorRotation().Yaw, 0.f);
}

bool APropBase::IsFalling() const
{
    return StaticMesh && StaticMesh->IsSimulatingPhysics();
}

bool APropBase::IsAirborne_Server(float CheckDistance, float* OutGroundDistance) const
{
    if (!StaticMesh || !GetWorld())
    {
        return false;
    }

    // 프롭 바닥면 크기의 얇은 박스를 아래로 쓸어 받침이 있는지 본다(모서리만 걸쳐도 받침으로 인정).
    const FBoxSphereBounds Bounds = StaticMesh->Bounds;
    const float BottomZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
    const FVector HalfExtent(
        FMath::Max(Bounds.BoxExtent.X * 0.9f, 1.f),
        FMath::Max(Bounds.BoxExtent.Y * 0.9f, 1.f),
        1.f);
    const FVector Start(Bounds.Origin.X, Bounds.Origin.Y, BottomZ + 2.f);
    const FVector End = Start - FVector(0.f, 0.f, CheckDistance + 2.f);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(PropAirborneCheck), false, this);
    if (const AActor* AttachParent = GetAttachParentActor())
    {
        Params.AddIgnoredActor(AttachParent);
    }

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

    FHitResult Hit;
    const bool bHit = GetWorld()->SweepSingleByObjectType(
        Hit, Start, End, FQuat::Identity, ObjectParams, FCollisionShape::MakeBox(HalfExtent), Params);

    if (OutGroundDistance)
    {
        *OutGroundDistance = bHit ? FMath::Max(0.f, static_cast<float>(Start.Z - Hit.Location.Z) - 2.f) : -1.f;
    }
    return !bHit;
}

void APropBase::StartFallingIfAirborne_Server()
{
    if (!HasAuthority() || !StaticMesh || bPossessedNet || !GetWorld())
    {
        return;
    }

    if (!IsAirborne_Server(AirborneCheckDistance))
    {
        return;
    }

    // 떨어지면서 캐릭터를 누르거나 캐릭터 위에 얹히지 않도록 낙하 중에는 캐릭터와 충돌하지 않는다.
    SetPawnCollisionSuppressed_Server(true);

    StaticMesh->SetEnableGravity(true);
    StaticMesh->SetSimulatePhysics(true);

    if (!StaticMesh->IsSimulatingPhysics())
    {
        // 단순 콜리전이 없는 메시 등 물리 시뮬레이션이 불가능하면 바닥으로 바로 내려놓는다.
        StaticMesh->SetEnableGravity(false);

        float GroundDistance = -1.f;
        IsAirborne_Server(10000.f, &GroundDistance);
        if (GroundDistance > 0.f)
        {
            AddActorWorldOffset(FVector(0.f, 0.f, -GroundDistance), false, nullptr, ETeleportType::TeleportPhysics);
            ForceNetUpdate();
        }
        UE_LOG(LogTemp, Warning, TEXT("%s cannot simulate physics (no simple collision?). Snapped to ground instead."), *GetName());
        StopFalling_Server();
        return;
    }

    StaticMesh->WakeRigidBody();
    FallStartTime = GetWorld()->GetTimeSeconds();
    GetWorldTimerManager().SetTimer(TH_FallSettle, this, &APropBase::CheckFallSettled_Server, 0.2f, true);
    ForceNetUpdate();
}

void APropBase::CheckFallSettled_Server()
{
    if (!StaticMesh || !GetWorld() || !StaticMesh->IsSimulatingPhysics() || bPossessedNet)
    {
        // 낙하 중 다시 빙의되었거나 이미 꺼진 경우
        GetWorldTimerManager().ClearTimer(TH_FallSettle);
        return;
    }

    const double Elapsed = GetWorld()->GetTimeSeconds() - FallStartTime;
    const bool bSlow = StaticMesh->GetPhysicsLinearVelocity().Size() < SettleSpeedThreshold
        && StaticMesh->GetPhysicsAngularVelocityInDegrees().Size() < SettleSpeedThreshold;

    if (!StaticMesh->RigidBodyIsAwake() || (bSlow && Elapsed > 0.3) || Elapsed > MaxFallSeconds)
    {
        StopFalling_Server();
    }
}

void APropBase::StopFalling_Server()
{
    GetWorldTimerManager().ClearTimer(TH_FallSettle);

    if (!StaticMesh)
    {
        return;
    }

    StaticMesh->SetSimulatePhysics(false);
    StaticMesh->SetEnableGravity(false);
    ForceNetUpdate();

    // 착지 지점에 캐릭터가 있으면 겹친 채로 충돌을 켜면 캐릭터가 끼므로, 빠져나갈 때까지 기다린다.
    CheckPawnOverlapCleared_Server();
    if (bPawnCollisionSuppressed && GetWorld())
    {
        GetWorldTimerManager().SetTimer(TH_PawnOverlap, this, &APropBase::CheckPawnOverlapCleared_Server, 0.2f, true);
    }
}

void APropBase::CheckPawnOverlapCleared_Server()
{
    if (!HasAuthority())
    {
        return;
    }

    if (bPossessedNet || !IsOverlappingAnyPawn())
    {
        if (GetWorld())
        {
            GetWorldTimerManager().ClearTimer(TH_PawnOverlap);
        }
        SetPawnCollisionSuppressed_Server(false);
    }
}

bool APropBase::IsOverlappingAnyPawn() const
{
    if (!StaticMesh || !GetWorld())
    {
        return false;
    }

    TArray<FOverlapResult> Overlaps;
    FComponentQueryParams Params(SCENE_QUERY_STAT(PropPawnOverlap), this);
    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

    GetWorld()->ComponentOverlapMulti(
        Overlaps,
        StaticMesh,
        StaticMesh->GetComponentLocation(),
        StaticMesh->GetComponentQuat(),
        Params,
        ObjectParams);

    for (const FOverlapResult& Overlap : Overlaps)
    {
        if (Overlap.GetActor() && Overlap.GetActor() != this)
        {
            return true;
        }
    }
    return false;
}

void APropBase::SetPawnCollisionSuppressed_Server(bool bSuppressed)
{
    if (!HasAuthority() || bPawnCollisionSuppressed == bSuppressed)
    {
        return;
    }

    bPawnCollisionSuppressed = bSuppressed;
    ApplyPawnCollisionResponse();
    ForceNetUpdate();
}

void APropBase::OnRep_PawnCollisionSuppressed()
{
    ApplyPawnCollisionResponse();
}

void APropBase::ApplyPawnCollisionResponse()
{
    // 빙의 중일 때의 충돌 설정은 SetPossessedState(true)가 관리한다.
    if (!StaticMesh || bPossessedNet)
    {
        return;
    }

    StaticMesh->SetCollisionResponseToChannel(ECC_Pawn, bPawnCollisionSuppressed ? ECR_Ignore : ECR_Block);
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
