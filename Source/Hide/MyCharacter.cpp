#include "MyCharacter.h"
#include "MyGameState.h"
#include "MyGameMode.h"
#include "MyPlayerController.h"
#include "SeekerProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PropBase.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

AMyCharacter::AMyCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->bEnablePhysicsInteraction = false;
    }
}

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AMyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMyCharacter, PossessedProp);
    DOREPLIFETIME(AMyCharacter, bLocked);
    DOREPLIFETIME(AMyCharacter, bForcedPossessLock);
    DOREPLIFETIME(AMyCharacter, bIsSeeker);
    DOREPLIFETIME(AMyCharacter, bIsDestroying);
    DOREPLIFETIME(AMyCharacter, bDestroyOnCooldown);
    DOREPLIFETIME(AMyCharacter, bSeekerAttackOnCooldown);
    DOREPLIFETIME(AMyCharacter, bEliminated);
    DOREPLIFETIME(AMyCharacter, SeekerHitCount);
}

void AMyCharacter::RequestPossessProp(APropBase* Prop)
{
    if (bIsSeeker) return;
    if (!Prop) return;

    if (HasAuthority())
    {
        ServerTryPossess(Prop);
        return;
    }

    ServerTryPossessByPropId(Prop->GetPropId());
}


void AMyCharacter::RequestReleaseProp()
{
    ServerReleaseProp();
}

void AMyCharacter::ForceReleasePossessedProp_Server()
{
    if (!HasAuthority()) return;
    ReleasePossessedPropInternal(true);
}

void AMyCharacter::ToggleLocked()
{
    ServerToggleLocked();
}

void AMyCharacter::RequestRotateLocked(float Dir)
{
    if (FMath::IsNearlyZero(Dir)) return;

    // 로컬 즉시 반영 (본인 화면용)
    if (IsLocallyControlled() && PossessedProp && bLocked)
    {
        const float Dt = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
        const float YawDelta = Dir * LockedRotateDegPerSec * Dt;
        AddActorWorldRotation(FRotator(0.f, YawDelta, 0.f));
    }

    // 서버 권위 반영
    ServerRotateLocked(Dir);
}


float AMyCharacter::GetCapsuleHalfHeight() const
{
    const UCapsuleComponent* Cap = GetCapsuleComponent();
    return Cap ? Cap->GetScaledCapsuleHalfHeight() : 0.f;
}

FVector AMyCharacter::PropWorldLocationForCharacter(APropBase* Prop) const
{
    if (!Prop) return GetActorLocation();

    const float CapsuleHalf = GetCapsuleHalfHeight();
    const float PropBottomOffset = Prop->GetBottomOffsetFromActorLocation();

    // 캐릭터 바닥 기준에 프롭 바닥이 맞도록 Z 오프셋 조정
    return GetActorLocation() + FVector(0.f, 0.f, PropBottomOffset - CapsuleHalf);
}

bool AMyCharacter::FindCharacterReleaseLocation(APropBase* Prop, FVector& OutLocation) const
{
    if (!Prop || !GetWorld()) return false;

    const UStaticMeshComponent* PropMesh = Prop->GetStaticMesh();
    const UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (!PropMesh || !Capsule) return false;

    const FVector Forward = GetActorForwardVector();
    const FVector Right = GetActorRightVector();
    const FVector BoundsCenter = PropMesh->Bounds.Origin;
    const FVector PropExtent = PropMesh->Bounds.BoxExtent;
    const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();

    const TArray<FVector> Directions = {
        Forward,
        (Forward + Right).GetSafeNormal(),
        (Forward - Right).GetSafeNormal(),
        Right,
        -Right,
        -Forward,
        (-Forward + Right).GetSafeNormal(),
        (-Forward - Right).GetSafeNormal()
    };

    for (const float ExtraDistance : { 0.f, 30.f, 60.f, 100.f })
    {
        FVector BestCandidate = FVector::ZeroVector;
        float BestDistanceSquared = MAX_flt;
        bool bFoundCandidate = false;

        for (const FVector& Direction : Directions)
        {
            const float PropRadiusInDirection = FMath::Abs(Direction.X) * PropExtent.X
                + FMath::Abs(Direction.Y) * PropExtent.Y;
            const float ReleaseDistance = PropRadiusInDirection
                + CapsuleRadius
                + ReleaseClearanceMargin
                + ExtraDistance;

            const FVector Candidate(
                BoundsCenter.X + Direction.X * ReleaseDistance,
                BoundsCenter.Y + Direction.Y * ReleaseDistance,
                GetActorLocation().Z);
            const FVector GroundedCandidate = ProjectCharacterReleaseLocationToGround(Prop, Candidate);
            if (IsCharacterReleaseLocationClear(Prop, GroundedCandidate))
            {
                const FVector CandidateDelta = GroundedCandidate - GetActorLocation();
                const float DistanceSquared = FMath::Square(CandidateDelta.X)
                    + FMath::Square(CandidateDelta.Y);
                if (!bFoundCandidate || DistanceSquared < BestDistanceSquared)
                {
                    BestCandidate = GroundedCandidate;
                    BestDistanceSquared = DistanceSquared;
                    bFoundCandidate = true;
                }
            }
        }

        if (bFoundCandidate)
        {
            OutLocation = BestCandidate;
            return true;
        }
    }

    return false;
}

FVector AMyCharacter::ProjectCharacterReleaseLocationToGround(APropBase* Prop, const FVector& Location) const
{
    if (!Prop || !GetWorld())
    {
        return Location;
    }

    const UStaticMeshComponent* PropMesh = Prop->GetStaticMesh();
    const float CapsuleHalf = GetCapsuleHalfHeight();
    const float PropTop = PropMesh
        ? PropMesh->Bounds.Origin.Z + PropMesh->Bounds.BoxExtent.Z
        : GetActorLocation().Z;
    const float TraceStartZ = FMath::Max(GetActorLocation().Z + CapsuleHalf + 100.f, PropTop + 100.f);
    const FVector TraceStart(Location.X, Location.Y, TraceStartZ);
    const FVector TraceEnd(Location.X, Location.Y, TraceStartZ - 3000.f);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PropReleaseGroundTrace), false);
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(Prop);

    const bool bHitGround = GetWorld()->LineTraceSingleByChannel(
        Hit,
        TraceStart,
        TraceEnd,
        ECC_Visibility,
        Params
    );

    if (!bHitGround)
    {
        return Location;
    }

    return FVector(Location.X, Location.Y, Hit.ImpactPoint.Z + CapsuleHalf + 2.f);
}

bool AMyCharacter::IsCharacterReleaseLocationClear(APropBase* Prop, const FVector& Location) const
{
    if (!Prop || !GetWorld())
    {
        return false;
    }

    const UStaticMeshComponent* PropMesh = Prop->GetStaticMesh();
    if (!PropMesh)
    {
        return false;
    }

    const UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (!Capsule)
    {
        return false;
    }

    const FVector PropExtent = PropMesh->Bounds.BoxExtent;
    const FVector CharacterDelta = Location - PropMesh->Bounds.Origin;
    const float SeparationX = FMath::Max(FMath::Abs(CharacterDelta.X) - PropExtent.X, 0.f);
    const float SeparationY = FMath::Max(FMath::Abs(CharacterDelta.Y) - PropExtent.Y, 0.f);
    const float RequiredClearance = Capsule->GetScaledCapsuleRadius() + ReleaseClearanceMargin * 0.5f;
    if (FMath::Square(SeparationX) + FMath::Square(SeparationY) < FMath::Square(RequiredClearance))
    {
        return false;
    }

    const FCollisionShape Shape = FCollisionShape::MakeCapsule(
        Capsule->GetScaledCapsuleRadius() * 0.95f,
        Capsule->GetScaledCapsuleHalfHeight() * 0.95f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CharacterReleaseOverlap), false);
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(Prop);

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

    return !GetWorld()->OverlapAnyTestByObjectType(
        Location,
        FQuat::Identity,
        ObjectParams,
        Shape,
        Params
    );
}

void AMyCharacter::ServerReleaseProp_Implementation()
{
    ReleasePossessedPropInternal(false);
}

void AMyCharacter::ReleasePossessedPropInternal(bool bIgnoreForcedPossessLock)
{
    if (bForcedPossessLock && !bIgnoreForcedPossessLock) return;
    if (!PossessedProp) return;

    APropBase* Old = PossessedProp;
    FVector CharacterReleaseLocation;
    if (!FindCharacterReleaseLocation(Old, CharacterReleaseLocation))
    {
        UE_LOG(LogTemp, Warning, TEXT("Release cancelled: no clear character location around %s"), *GetNameSafe(Old));
        return;
    }

    const FVector PropLocation = Old->GetActorLocation();
    const FRotator PropRotation = Old->GetActorRotation();
    Old->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetActorLocation(CharacterReleaseLocation, false, nullptr, ETeleportType::TeleportPhysics);

    PossessedProp = nullptr;
    bLocked = false;
    bForcedPossessLock = false;

    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(TH_ForcedPossessLock);
    }

    if (Old)
    {
        Old->FinalizeReleaseTransform(PropLocation, PropRotation);
        Old->MulticastFinalizeReleaseTransform(PropLocation, PropRotation);
        Old->SetPossessedNet(false);
        Old->ReleaseClaim(this);
        Old->ForceNetUpdate();
    }

    OnRep_PossessedProp();
    OnRep_Locked();
    ForceNetUpdate();

    if (AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr)
    {
        if (GS->GetGamePhase() == EGamePhase::ForcedSwap)
        {
            if (AMyPlayerController* MPC = Cast<AMyPlayerController>(GetController()))
            {
                MPC->ClientSetForcedSwapWarning(false, 0.0);
            }
        }
    }
}

void AMyCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->JumpZVelocity *= FMath::Max(0.1f, JumpStrengthMultiplier);
    }
}

void AMyCharacter::EndForcedPossessLock()
{
    if (!HasAuthority()) return;

    bForcedPossessLock = false;
    ForceNetUpdate();
}

void AMyCharacter::ServerToggleLocked_Implementation()
{
    if (!PossessedProp)
    {
        bLocked = false;
        OnRep_Locked();
        ForceNetUpdate();
        return;
    }

    bLocked = !bLocked;

    OnRep_Locked();
    ForceNetUpdate();
}

void AMyCharacter::ServerRotateLocked_Implementation(float Dir)
{
    if (!PossessedProp) return;
    if (!bLocked) return;
    if (FMath::IsNearlyZero(Dir)) return;

    const float Dt = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
    const float YawDelta = Dir * LockedRotateDegPerSec * Dt;

    AddActorWorldRotation(FRotator(0.f, YawDelta, 0.f));
    ForceNetUpdate();
}

void AMyCharacter::OnRep_PossessedProp()
{
    ApplyPossessVisuals(PossessedProp != nullptr);
    ApplyMoveIgnoreForProp(PossessedProp);
}

void AMyCharacter::OnRep_Locked()
{
    UCharacterMovementComponent* Move = GetCharacterMovement();
    if (!Move) return;

    if (PossessedProp && bLocked)
    {
        Move->StopMovementImmediately();
        Move->DisableMovement();
    }
    else
    {
        Move->SetMovementMode(EMovementMode::MOVE_Walking);
    }
}

void AMyCharacter::ApplyPossessVisuals(bool bPossessingNow)
{
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetHiddenInGame(bPossessingNow, true);
        MeshComp->SetVisibility(!bPossessingNow, true);
    }
}

void AMyCharacter::ApplyMoveIgnoreForProp(APropBase* NewProp)
{
    if (LastPossessedProp && LastPossessedProp != NewProp)
    {
        MoveIgnoreActorRemove(LastPossessedProp);
    }

    if (NewProp)
    {
        MoveIgnoreActorAdd(NewProp);
    }

    LastPossessedProp = NewProp;
}


void AMyCharacter::ServerRequestPossess_Implementation()
{
    if (bIsSeeker) return;

    FVector Start = GetActorLocation();
    FVector End = Start + GetActorForwardVector() * MaxPossessDistance;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    if (!bHit) return;

    APropBase* Prop = Cast<APropBase>(Hit.GetActor());
    if (!Prop) return;

    ServerTryPossess(Prop);
}

void AMyCharacter::ServerTryPossess_Implementation(APropBase* Prop)
{
    if (!IsValid(Prop)) return;
    if (Prop->IsPossessionBanned()) return;

    const float Dist = FVector::Dist(GetActorLocation(), Prop->GetActorLocation());
    if (Dist > MaxPossessDistance) return;

    // 이미 빙의 중이면 먼저 해제
    if (PossessedProp)
    {
        ServerReleaseProp();
        if (PossessedProp)
        {
            return;
        }
    }

    // Claim(예약)부터
    if (!Prop->TryClaim(this)) return;

    PossessedProp = Prop;
    bLocked = false;

    AttachAndSyncPossess(Prop);

    // 서버(리스닝 서버 포함) 화면 즉시 반영
    OnRep_PossessedProp();
    OnRep_Locked();

    ForceNetUpdate();
    Prop->ForceNetUpdate();

    if (AMyGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMyGameMode>() : nullptr)
    {
        GM->RegisterPhase1PossessedProp(Prop);

        if (AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr)
        {
            if (GS->GetGamePhase() == EGamePhase::ForcedSwap && GM->IsPhase1PossessedProp(Prop))
            {
                if (AMyPlayerController* MPC = Cast<AMyPlayerController>(GetController()))
                {
                    MPC->ClientSetForcedSwapWarning(true, GS->GetPhaseRemainingSeconds());
                }
            }
        }
    }
}

void AMyCharacter::ServerTryPossessByPropId_Implementation(FName PropId)
{
    if (PropId.IsNone() || !GetWorld())
    {
        return;
    }

    TArray<AActor*> FoundProps;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APropBase::StaticClass(), FoundProps);

    for (AActor* Actor : FoundProps)
    {
        APropBase* Prop = Cast<APropBase>(Actor);
        if (Prop && Prop->GetPropId() == PropId)
        {
            ServerTryPossess(Prop);
            return;
        }
    }
}

bool AMyCharacter::ForceReplacePossessedProp_Server(TSubclassOf<APropBase> NewPropClass)
{
    if (!HasAuthority()) return false;
    if (bIsSeeker) return false;
    if (!PossessedProp) return false;
    if (!NewPropClass) return false;
    if (!GetWorld()) return false;

    const FRotator SpawnRot = PossessedProp->GetActorRotation();

    ReleasePossessedPropInternal(true);
    if (PossessedProp)
    {
        return false;
    }

    FActorSpawnParameters Params;
    Params.Owner = GetController();
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    const FVector SpawnLoc = GetActorLocation();
    APropBase* NewProp = GetWorld()->SpawnActor<APropBase>(NewPropClass, SpawnLoc, SpawnRot, Params);
    if (!NewProp)
    {
        return false;
    }

    ServerTryPossess(NewProp);
    if (PossessedProp == NewProp && ForcedPossessLockSeconds > 0.0f)
    {
        bForcedPossessLock = true;
        GetWorldTimerManager().SetTimer(
            TH_ForcedPossessLock,
            this,
            &AMyCharacter::EndForcedPossessLock,
            ForcedPossessLockSeconds,
            false
        );
        ForceNetUpdate();
    }

    return PossessedProp == NewProp;
}


void AMyCharacter::AttachAndSyncPossess(APropBase* Prop)
{
    if (!Prop) return;

    Prop->SetPossessedNet(true);
    Prop->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepWorldTransform);
    Prop->SetActorLocation(PropWorldLocationForCharacter(Prop));
}

void AMyCharacter::SetIsSeeker_Server(bool bNewIsSeeker)
{
    if (!HasAuthority())
    {
        return;
    }

    bIsSeeker = bNewIsSeeker;

    if (bIsSeeker)
    {
        if (PossessedProp)
        {
            ReleasePossessedPropInternal(true);
        }
        bLocked = false;
        OnRep_Locked();
    }

    OnRep_IsSeeker();
    ForceNetUpdate();
}

void AMyCharacter::OnRep_IsSeeker()
{
}

void AMyCharacter::RequestDestroyPropSkill(APropBase* Prop)
{
    if (bEliminated) return;
    if (!Prop) return;

    ServerStartDestroyPropSkill(Prop);
}   

void AMyCharacter::ServerStartDestroyPropSkill_Implementation(APropBase* Prop)
{
    if (!HasAuthority()) return;
    if (bEliminated) return;
    if (!IsValid(Prop)) return;

    if (bIsDestroying) return;
    if (bDestroyOnCooldown) return;

    // 서버에서 거리 검증 (치팅 방지)
    const float Dist = FVector::Dist(GetActorLocation(), Prop->GetActorLocation());
    if (Dist > DestroySkillRange) return;

    if (Prop->IsPossessedNet()) return;

    bIsDestroying = true;
    DestroyTargetProp = Prop;

    bDestroyOnCooldown = true;

    GetWorldTimerManager().SetTimer(TH_DestroyFinish, this, &AMyCharacter::FinishDestroyPropSkill, DestroyTimeSeconds, false);
    GetWorldTimerManager().SetTimer(TH_DestroyCooldown, this, &AMyCharacter::EndDestroyCooldown, DestroyCooldownSeconds, false);

    OnRep_DestroySkillState();
    ForceNetUpdate();
}

void AMyCharacter::FinishDestroyPropSkill()
{
    if (!HasAuthority()) return;

    bIsDestroying = false;

    APropBase* Target = DestroyTargetProp;
    DestroyTargetProp = nullptr;

    if (!IsValid(Target))
    {
        OnRep_DestroySkillState();
        ForceNetUpdate();
        return;
    }

    // 완료 시점 거리 재검증(멀어졌으면 실패 처리)
    const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
    if (Dist > DestroySkillRange)
    {
        OnRep_DestroySkillState();
        ForceNetUpdate();
        return;
    }

    // 파괴 기록 저장 (탈락자가 생성할 목록은 GameState RespawnList로 가정)
    if (AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr)
    {
        GS->RegisterDestroyedProp(Target);
        GS->TrySpawnClueFromDestroyedProp(Target);
    }

    Target->Destroy();

    OnRep_DestroySkillState();
    ForceNetUpdate();
}

void AMyCharacter::EndDestroyCooldown()
{
    if (!HasAuthority()) return;

    bDestroyOnCooldown = false;
    ForceNetUpdate();
}

void AMyCharacter::OnRep_DestroySkillState()
{
    // UI/이펙트 연결
    // 예: bIsDestroying true면 3초 게이지 표시 등
}

void AMyCharacter::RequestSeekerAttack()
{
    if (bEliminated) return;
    if (!bIsSeeker) return;

    ServerSeekerAttack();
}

void AMyCharacter::ServerSeekerAttack_Implementation()
{
    if (!HasAuthority())
    {
        return;
    }

    if (bEliminated)
    {
        return;
    }

    if (!bIsSeeker)
    {
        return;
    }

    if (bSeekerAttackOnCooldown)
    {
        return;
    }

    if (!SeekerProjectileClass)
    {
        return;
    }

    FVector ViewLoc = GetActorLocation();
    FRotator ViewRot = Controller ? Controller->GetControlRotation() : GetActorRotation();
    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        PC->GetPlayerViewPoint(ViewLoc, ViewRot);
    }

    FHitResult AimHit;
    FCollisionQueryParams AimParams(SCENE_QUERY_STAT(SeekerAimTrace), true);
    AimParams.AddIgnoredActor(this);

    const FVector AimTraceStart = ViewLoc;
    const FVector AimTraceEnd = AimTraceStart + ViewRot.Vector() * 10000.f;
    const bool bAimHit = GetWorld()->LineTraceSingleByChannel(
        AimHit,
        AimTraceStart,
        AimTraceEnd,
        ECC_Visibility,
        AimParams
    );

    const FVector AimPoint = bAimHit ? AimHit.ImpactPoint : AimTraceEnd;
    const FVector Forward = ViewRot.Vector();
    const float CapsuleRadius = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleRadius() : 34.f;
    const FVector SpawnLoc = GetActorLocation() + FVector(0.f, 0.f, 60.f) + Forward * (CapsuleRadius + 8.f);
    const FRotator SpawnRot = (AimPoint - SpawnLoc).Rotation();

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.Instigator = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ASeekerProjectile* Proj = GetWorld()->SpawnActor<ASeekerProjectile>(
        SeekerProjectileClass,
        SpawnLoc,
        SpawnRot,
        Params
    );

    bSeekerAttackOnCooldown = true;

    if (SeekerAttackCooldownSeconds > 0.f)
    {
        GetWorldTimerManager().SetTimer(
            TH_SeekerAttackCooldown,
            this,
            &AMyCharacter::EndSeekerAttackCooldown,
            SeekerAttackCooldownSeconds,
            false
        );
    }

    ForceNetUpdate();
}

void AMyCharacter::EndSeekerAttackCooldown()
{
    if (!HasAuthority()) return;
    bSeekerAttackOnCooldown = false;
    ForceNetUpdate();
}

void AMyCharacter::SetEliminated_Server(bool bNewEliminated)
{
    if (!HasAuthority()) return;

    bEliminated = bNewEliminated;

    if (bEliminated)
    {
        ApplyEliminatedVisuals();
    }

    ForceNetUpdate();
}

void AMyCharacter::OnRep_Eliminated()
{
    if (!bEliminated) return;

    if (PossessedProp)
    {
        APropBase* OldProp = PossessedProp;
        DetachAndSyncRelease(OldProp);
    }

    bLocked = false;
    bIsDestroying = false;
    DestroyTargetProp = nullptr;

    ApplyEliminatedVisuals();

    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetController()))
    {
        PC->HideGameRoomUI();
    }
}

void AMyCharacter::ApplySeekerHit_Server()
{
    if (!HasAuthority()) return;
    if (bEliminated) return;
    if (bIsSeeker) return;

    ++SeekerHitCount;
    OnRep_SeekerHitCount();
    ForceNetUpdate();

    if (SeekerHitCount >= MaxSeekerHitsBeforeElimination)
    {
        SetEliminated_Server(true);

        if (AMyGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMyGameMode>() : nullptr)
        {
            GM->HandleRunnerEliminated(this);
        }
    }
}

void AMyCharacter::OnRep_SeekerHitCount()
{
    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetController()))
    {
        PC->UpdateGameRoomHearts(SeekerHitCount);
    }
}

void AMyCharacter::DetachAndSyncRelease(APropBase* Prop)
{
    if (!Prop) return;

    const FVector PropLocation = Prop->GetActorLocation();
    const FRotator PropRotation = Prop->GetActorRotation();

    if (HasAuthority())
    {
        FVector CharacterReleaseLocation;
        if (!FindCharacterReleaseLocation(Prop, CharacterReleaseLocation))
        {
            return;
        }

        Prop->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        SetActorLocation(CharacterReleaseLocation, false, nullptr, ETeleportType::TeleportPhysics);
        Prop->FinalizeReleaseTransform(PropLocation, PropRotation);
        Prop->ReleaseClaim(this);
        Prop->MulticastFinalizeReleaseTransform(PropLocation, PropRotation);
        Prop->SetPossessedNet(false);
        Prop->ForceNetUpdate();
    }
    else
    {
        Prop->FinalizeReleaseTransform(PropLocation, PropRotation);
        Prop->SetPossessedState(false);
    }

    if (PossessedProp == Prop)
    {
        PossessedProp = nullptr;
    }

    if (LastPossessedProp == Prop)
    {
        LastPossessedProp = nullptr;
    }

    ApplyPossessVisuals(false);
}

void AMyCharacter::ApplyEliminatedVisuals()
{
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
    }

    SetActorEnableCollision(false);

    if (GetCapsuleComponent())
    {
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetCapsuleComponent()->SetHiddenInGame(true, true);
        GetCapsuleComponent()->SetVisibility(false, true);
    }

    if (GetMesh())
    {
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetMesh()->SetVisibility(false, true);
        GetMesh()->SetHiddenInGame(true, true);
        GetMesh()->SetOwnerNoSee(true);
        GetMesh()->bCastHiddenShadow = false;
    }

    TArray<USceneComponent*> ChildComponents;
    if (GetRootComponent())
    {
        GetRootComponent()->GetChildrenComponents(true, ChildComponents);
    }

    for (USceneComponent* ChildComp : ChildComponents)
    {
        if (!ChildComp) continue;

        ChildComp->SetHiddenInGame(true, true);

        if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(ChildComp))
        {
            Prim->SetVisibility(false, true);
            Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

    SetActorHiddenInGame(true);
}

