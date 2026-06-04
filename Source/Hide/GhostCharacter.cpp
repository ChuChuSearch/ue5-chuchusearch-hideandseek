#include "GhostCharacter.h"
#include "GhostPlacementPreview.h"
#include "MyPlayerController.h"
#include "MyGameState.h"
#include "MyCharacter.h"
#include "PropBase.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AGhostCharacter::AGhostCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);

    GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    GetCharacterMovement()->GravityScale = 0.f;

    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

    bAlwaysRelevant = false;
    NetUpdateFrequency = 33.f;
    MinNetUpdateFrequency = 15.f;
}

void AGhostCharacter::BeginPlay()
{
    Super::BeginPlay();

    GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    RefreshLocalGhostVisibility();
}

void AGhostCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (IsLocallyControlled() && bPlacementMode)
    {
        UpdatePlacementRotation(DeltaSeconds);
        UpdatePlacementPreview();
    }
}

void AGhostCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AGhostCharacter, bPlacementMode);
    DOREPLIFETIME(AGhostCharacter, PlacedCount);
    DOREPLIFETIME(AGhostCharacter, PlacementCooldownSeconds);
    DOREPLIFETIME(AGhostCharacter, LastPlacementServerWorldTime);
}

bool AGhostCharacter::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
    const APlayerController* ViewerPC = Cast<APlayerController>(RealViewer);
    if (!ViewerPC)
    {
        return false;
    }

    const APawn* ViewerPawn = ViewerPC->GetPawn();
    if (!ViewerPawn)
    {
        return false;
    }

    // 고스트인 플레이어에게만 고스트를 보여줌
    return ViewerPawn->IsA(AGhostCharacter::StaticClass());
}

void AGhostCharacter::TogglePlacementMode()
{
    if (!IsLocallyControlled()) return;

    bPlacementMode = !bPlacementMode;
    ApplyPlacementModeInput(bPlacementMode);

    if (!bPlacementMode)
    {
        DestroyPreview();
        SelectedRespawnIndex = INDEX_NONE;
        SelectedPropClass = nullptr;
        SelectedPreviewMesh = nullptr;
        SelectedPreviewScale = FVector::OneVector;
        return;
    }

    RebuildPreviewFromSelection();
}

void AGhostCharacter::CancelPlacementMode()
{
    if (!IsLocallyControlled())
    {
        return;
    }

    bPlacementMode = false;
    ApplyPlacementModeInput(false);
    ClearPlacementSelection();
}

void AGhostCharacter::ApplyPlacementModeInput(bool bEnable)
{
    AMyPlayerController* PC = Cast<AMyPlayerController>(GetController());
    if (!PC) return;

    PC->bShowMouseCursor = bEnable;
    PC->bEnableClickEvents = bEnable;
    PC->bEnableMouseOverEvents = bEnable;

    if (bEnable)
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode);
    }
    else
    {
        if (PC->IsGhostUIOpen())
        {
            FInputModeGameAndUI Mode;
            Mode.SetHideCursorDuringCapture(false);
            Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PC->SetInputMode(Mode);
            PC->bShowMouseCursor = true;
            PC->bEnableClickEvents = true;
            PC->bEnableMouseOverEvents = true;
        }
        else
        {
            FInputModeGameOnly Mode;
            PC->SetInputMode(Mode);
            PC->bShowMouseCursor = false;
        }
    }
}

void AGhostCharacter::SetSelectedRespawnIndex(int32 InIndex)
{
    SelectedRespawnIndex = InIndex;

    const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    if (!GS || !GS->IsValidRespawnIndex(SelectedRespawnIndex))
    {
        SelectedRespawnIndex = INDEX_NONE;
        SelectedPropClass = nullptr;
        SelectedPreviewMesh = nullptr;
        SelectedPreviewScale = FVector::OneVector;
        DestroyPreview();
        return;
    }

    const FRespawnPropInfo& Info = GS->GetRespawnList()[SelectedRespawnIndex];
    SelectedPropClass = Info.PropClass;

    SelectedPreviewMesh = nullptr;
    SelectedPreviewScale = Info.Transform.GetScale3D();

    if (SelectedPropClass)
    {
        if (const APropBase* DefaultProp = Cast<APropBase>(SelectedPropClass->GetDefaultObject()))
        {
            if (DefaultProp->GetStaticMesh())
            {
                SelectedPreviewMesh = DefaultProp->GetStaticMesh()->GetStaticMesh();
            }
        }
    }

    RebuildPreviewFromSelection();
}

void AGhostCharacter::RebuildPreviewFromSelection()
{
    if (!IsLocallyControlled()) return;

    if (!bPlacementMode || !SelectedPropClass || !SelectedPreviewMesh)
    {
        DestroyPreview();
        return;
    }

    if (!PlacementPreviewActor && PlacementPreviewClass)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        PlacementPreviewActor = GetWorld()->SpawnActor<AGhostPlacementPreview>(
            PlacementPreviewClass,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            Params
        );
    }

    if (PlacementPreviewActor)
    {
        PlacementPreviewActor->SetPreviewMesh(SelectedPreviewMesh, SelectedPreviewScale);
        PlacementPreviewActor->SetActorHiddenInGame(false);
    }
}

void AGhostCharacter::DestroyPreview()
{
    if (PlacementPreviewActor)
    {
        PlacementPreviewActor->Destroy();
        PlacementPreviewActor = nullptr;
    }
}

bool AGhostCharacter::GetPlacementHit(FHitResult& OutHit) const
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return false;

    return PC->GetHitResultUnderCursorByChannel(
        UEngineTypes::ConvertToTraceType(ECC_Visibility),
        true,
        OutHit
    );
}

bool AGhostCharacter::IsNearAnyLivePlayer(const FVector& Location, float Radius) const
{
    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyCharacter::StaticClass(), Found);

    const float RadiusSq = Radius * Radius;
    for (AActor* Actor : Found)
    {
        const AMyCharacter* Char = Cast<AMyCharacter>(Actor);
        if (!Char) continue;
        if (Char->IsEliminated()) continue;

        if (FVector::DistSquared(Char->GetActorLocation(), Location) <= RadiusSq)
        {
            return true;
        }
    }

    return false;
}

bool AGhostCharacter::IsPlacementValid_Local(const FVector& Location, const FRotator& Rotation) const
{
    if (!SelectedPropClass || !SelectedPreviewMesh) return false;

    if (IsNearAnyLivePlayer(Location, PlayerBlockRadius))
    {
        return false;
    }

    FVector Extent = SelectedPreviewMesh->GetBounds().BoxExtent * SelectedPreviewScale.GetAbs();
    Extent *= 0.95f;

    FCollisionShape Shape = FCollisionShape::MakeBox(Extent);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(GhostPlacementOverlap), false);
    Params.AddIgnoredActor(this);

    const bool bOverlapStatic = GetWorld()->OverlapBlockingTestByChannel(
        Location,
        Rotation.Quaternion(),
        ECC_WorldStatic,
        Shape,
        Params
    );

    const bool bOverlapDynamic = GetWorld()->OverlapBlockingTestByChannel(
        Location,
        Rotation.Quaternion(),
        ECC_WorldDynamic,
        Shape,
        Params
    );

    if (bOverlapStatic || bOverlapDynamic)
    {
        return false;
    }

    return true;
}

void AGhostCharacter::UpdatePlacementRotation(float DeltaSeconds)
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }

    float Direction = 0.f;
    if (PC->IsInputKeyDown(EKeys::Q))
    {
        Direction -= 1.f;
    }
    if (PC->IsInputKeyDown(EKeys::E))
    {
        Direction += 1.f;
    }

    if (FMath::IsNearlyZero(Direction))
    {
        return;
    }

    PlacementYaw = FRotator::ClampAxis(PlacementYaw + Direction * PlacementRotationSpeedDegrees * DeltaSeconds);
}

void AGhostCharacter::UpdatePlacementPreview()
{
    const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;

    if (!GS || SelectedRespawnIndex == INDEX_NONE || !GS->IsValidRespawnIndex(SelectedRespawnIndex))
    {
        ClearPlacementSelection();
        return;
    }

    if (!PlacementPreviewActor || !SelectedPropClass || !SelectedPreviewMesh)
    {
        return;
    }

    FHitResult Hit;
    if (!GetPlacementHit(Hit))
    {
        PlacementPreviewActor->SetActorHiddenInGame(true);
        return;
    }

    if (Hit.ImpactNormal.Z < 0.75f)
    {
        PlacementPreviewActor->SetActorHiddenInGame(true);
        return;
    }

    FVector PlaceLocation = Hit.Location;
    FRotator PlaceRotation = FRotator(0.f, PlacementYaw, 0.f);

    if (PreviewGridSnap > 0.f)
    {
        PlaceLocation.X = FMath::GridSnap(PlaceLocation.X, PreviewGridSnap);
        PlaceLocation.Y = FMath::GridSnap(PlaceLocation.Y, PreviewGridSnap);
    }

    const FVector BoxExtent = SelectedPreviewMesh->GetBounds().BoxExtent * SelectedPreviewScale.GetAbs();
    PlaceLocation.Z += BoxExtent.Z + GroundSnapOffset;

    const bool bValid = IsPlacementValid_Local(PlaceLocation, PlaceRotation);

    PlacementPreviewActor->SetActorHiddenInGame(false);
    PlacementPreviewActor->SetPreviewTransform(PlaceLocation, PlaceRotation);
    PlacementPreviewActor->SetPlacementValid(bValid);
}

float AGhostCharacter::GetPlacementCooldownRemaining() const
{
    const UWorld* World = GetWorld();
    if (!World) return 0.f;

    const AGameStateBase* GS = World->GetGameState();
    const float Now = GS ? GS->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
    const float Elapsed = Now - LastPlacementServerWorldTime;
    return FMath::Max(0.f, PlacementCooldownSeconds - Elapsed);
}

void AGhostCharacter::StartLocalCooldownPrediction()
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const AGameStateBase* GS = World->GetGameState();
    LastPlacementServerWorldTime = GS ? GS->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

void AGhostCharacter::TryConfirmPlacement()
{
    if (!IsLocallyControlled()) return;
    if (!bPlacementMode) return;
    if (SelectedRespawnIndex == INDEX_NONE) return;
    if (GetPlacementCooldownRemaining() > 0.f) return;

    FHitResult Hit;
    if (!GetPlacementHit(Hit)) return;
    if (Hit.ImpactNormal.Z < 0.75f) return;

    FVector PlaceLocation = Hit.Location;
    FRotator PlaceRotation(0.f, PlacementYaw, 0.f);

    if (PreviewGridSnap > 0.f)
    {
        PlaceLocation.X = FMath::GridSnap(PlaceLocation.X, PreviewGridSnap);
        PlaceLocation.Y = FMath::GridSnap(PlaceLocation.Y, PreviewGridSnap);
    }

    if (SelectedPreviewMesh)
    {
        const FVector BoxExtent = SelectedPreviewMesh->GetBounds().BoxExtent * SelectedPreviewScale.GetAbs();
        PlaceLocation.Z += BoxExtent.Z + GroundSnapOffset;
    }

    if (!IsPlacementValid_Local(PlaceLocation, PlaceRotation))
    {
        return;
    }

    ServerRequestPlaceRespawnProp(SelectedRespawnIndex, PlaceLocation, PlaceRotation);
    StartLocalCooldownPrediction();

    // 배치 시도 후 즉시 프리뷰 제거 + 선택 해제
    ClearPlacementSelection();
    bPlacementMode = false;
    ApplyPlacementModeInput(false);
}

void AGhostCharacter::ServerRequestPlaceRespawnProp_Implementation(int32 RespawnIndex, FVector_NetQuantize Location, FRotator Rotation)
{
    if (!HasAuthority()) return;

    if (PlacementCooldownSeconds > 0.f)
    {
        const float Now = GetWorld()->GetTimeSeconds();
        if (Now - LastPlacementServerWorldTime < PlacementCooldownSeconds)
        {
            return;
        }
    }

    if (MaxPlacedObjects > 0 && PlacedCount >= MaxPlacedObjects)
    {
        return;
    }

    AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    if (!GS || !GS->IsValidRespawnIndex(RespawnIndex))
    {
        return;
    }

    // 근처 플레이어 금지
    if (IsNearAnyLivePlayer(Location, PlayerBlockRadius))
    {
        return;
    }

    FRespawnPropInfo Info;
    if (!GS->ConsumeRespawnEntry(RespawnIndex, Info))
    {
        return;
    }

    if (!Info.PropClass)
    {
        return;
    }

    // 바닥 위인지 서버도 다시 체크
    FHitResult GroundHit;
    const FVector TraceStart = Location + FVector(0.f, 0.f, 100.f);
    const FVector TraceEnd = Location - FVector(0.f, 0.f, 300.f);

    FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(GhostPlacementGroundTrace), false);
    TraceParams.AddIgnoredActor(this);

    const bool bGroundHit = GetWorld()->LineTraceSingleByChannel(
        GroundHit,
        TraceStart,
        TraceEnd,
        ECC_Visibility,
        TraceParams
    );

    if (!bGroundHit || GroundHit.ImpactNormal.Z < 0.75f)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.Instigator = this;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

    FTransform SpawnTransform(Rotation, Location, FVector::OneVector);

    AActor* Spawned = GetWorld()->SpawnActor<AActor>(
        Info.PropClass,
        SpawnTransform,
        Params
    );

    if (!Spawned)
    {
        GS->RestoreRespawnEntry(Info);
        return;
    }

    if (APropBase* SpawnedProp = Cast<APropBase>(Spawned))
    {
        if (UStaticMeshComponent* MeshComp = SpawnedProp->GetStaticMesh())
        {
            MeshComp->SetWorldScale3D(Info.Transform.GetScale3D());
        }

        SpawnedProp->SetReplicates(true);
        SpawnedProp->SetReplicateMovement(true);
    }
    else
    {
        Spawned->SetActorScale3D(Info.Transform.GetScale3D());
    }

    ++PlacedCount;
    LastPlacementServerWorldTime = GetWorld()->GetTimeSeconds();
    ForceNetUpdate();
}

void AGhostCharacter::TryRefreshRespawnList()
{
    if (!IsLocallyControlled())
    {
        return;
    }

    if (GetPlacementCooldownRemaining() > 0.f)
    {
        return;
    }

    const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    if (!GS || !GS->CanRefreshVisibleRespawnList())
    {
        return;
    }

    ServerRequestRefreshRespawnList();
    StartLocalCooldownPrediction();
    CancelPlacementMode();
}

void AGhostCharacter::ServerRequestRefreshRespawnList_Implementation()
{
    if (!HasAuthority())
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (PlacementCooldownSeconds > 0.f && Now - LastPlacementServerWorldTime < PlacementCooldownSeconds)
    {
        return;
    }

    AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    if (!GS || !GS->RequestRefreshVisibleRespawnList())
    {
        return;
    }

    LastPlacementServerWorldTime = Now;
    ForceNetUpdate();
}

void AGhostCharacter::RefreshLocalGhostVisibility()
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!PC) return;

    APawn* LocalPawn = PC->GetPawn();
    const bool bLocalIsGhost = LocalPawn && LocalPawn->IsA(AGhostCharacter::StaticClass());

    SetActorHiddenInGame(!bLocalIsGhost);
    SetActorEnableCollision(bLocalIsGhost);
}   

void AGhostCharacter::ClearPlacementSelection()
{
    SelectedRespawnIndex = INDEX_NONE;
    SelectedPropClass = nullptr;
    SelectedPreviewMesh = nullptr;
    SelectedPreviewScale = FVector::OneVector;
    PlacementYaw = 0.f;

    DestroyPreview();
}
