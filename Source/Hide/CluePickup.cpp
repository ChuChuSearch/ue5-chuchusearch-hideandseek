#include "CluePickup.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "MyCharacter.h"
#include "MyPlayerState.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACluePickup::ACluePickup()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);
    
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
    OverlapSphere->SetupAttachment(Root);
    OverlapSphere->SetSphereRadius(70.f);
    OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    OverlapSphere->SetCollisionObjectType(ECC_WorldDynamic);
    OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ACluePickup::OnOverlapBegin);

    ClueWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ClueWidget"));
    ClueWidget->SetupAttachment(Root);
    ClueWidget->SetWidgetSpace(EWidgetSpace::World);
    ClueWidget->SetDrawAtDesiredSize(false);
    ClueWidget->SetDrawSize(FVector2D(96.f, 96.f));
    ClueWidget->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.35f));
    ClueWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ClueWidget->SetTwoSided(true);

    // 빛기둥: 기본 원기둥(지름 100, 높이 100, 중심이 가운데)을 가늘고 길게 늘인다
    BeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeamMesh"));
    BeamMesh->SetupAttachment(Root);
    BeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BeamMesh->SetGenerateOverlapEvents(false);
    BeamMesh->SetCastShadow(false);
    BeamMesh->SetCanEverAffectNavigation(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> BeamMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (BeamMeshFinder.Succeeded())
    {
        BeamMesh->SetStaticMesh(BeamMeshFinder.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BeamMaterialFinder(TEXT("/Game/Game/Materials/M_CheatBeacon.M_CheatBeacon"));
    if (BeamMaterialFinder.Succeeded())
    {
        BeamMesh->SetMaterial(0, BeamMaterialFinder.Object);
    }

    GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
    GlowLight->SetupAttachment(Root);
    GlowLight->SetLightColor(FLinearColor(1.f, 0.82f, 0.35f));
    GlowLight->SetAttenuationRadius(450.f);
    GlowLight->SetCastShadows(false);
    GlowLight->SetIntensity(GlowIntensity);
}

void ACluePickup::BeginPlay()
{
    Super::BeginPlay();

    VisualAge = 0.f;
    IconBaseLocation = ClueWidget ? ClueWidget->GetRelativeLocation() : FVector::ZeroVector;

    if (ClueWidget)
    {
        // 등장 연출로 0에서 커지기 시작
        ClueWidget->SetRelativeScale3D(FVector(0.01f));
    }

    if (BeamMesh)
    {
        // 기둥은 단서 아이콘 위에서 시작해 위로 뻗는다(아이콘 절반 높이 + 둥실 높이 + 여유)
        const float IconHalfHeight = ClueWidget ? ClueWidget->GetDrawSize().Y * 0.5f * IconWorldScale : 48.f;
        const float BottomOffset = IconBaseLocation.Z + IconHalfHeight + BobHeight + 15.f;
        BeamMesh->SetRelativeScale3D(FVector(BeamWidth / 100.f, BeamWidth / 100.f, BeamHeight / 100.f));
        BeamMesh->SetRelativeLocation(FVector(0.f, 0.f, BottomOffset + BeamHeight * 0.5f));
    }

    if (GlowLight)
    {
        GlowLight->SetIntensity(GlowIntensity);
    }
}

void ACluePickup::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 연출은 화면이 있는 쪽에서만 의미가 있다
    if (GetNetMode() != NM_DedicatedServer)
    {
        UpdateVisualEffects(DeltaSeconds);
        FaceToLocalPlayerCamera();
    }
}

void ACluePickup::UpdateVisualEffects(float DeltaSeconds)
{
    VisualAge += DeltaSeconds;

    if (ClueWidget)
    {
        // 등장: 살짝 넘쳤다가 제자리(easeOutBack)
        const float PopAlpha = FMath::Clamp(VisualAge / PopInDuration, 0.f, 1.f);
        const float C1 = 1.70158f;
        const float C3 = C1 + 1.f;
        const float T = PopAlpha - 1.f;
        const float PopScale = 1.f + C3 * T * T * T + C1 * T * T;

        // 등장 후 숨 쉬듯 살짝 커졌다 작아졌다
        const float Breath = 1.f + 0.06f * FMath::Sin(VisualAge * 3.2f);
        const float Scale = FMath::Max(0.01f, IconWorldScale * PopScale * Breath);
        ClueWidget->SetRelativeScale3D(FVector(Scale));

        const float Bob = BobHeight * FMath::Sin(VisualAge * BobSpeed);
        ClueWidget->SetRelativeLocation(IconBaseLocation + FVector(0.f, 0.f, Bob));
    }

    if (GlowLight)
    {
        const float Pulse = 0.75f + 0.25f * FMath::Sin(VisualAge * 3.2f);
        GlowLight->SetIntensity(GlowIntensity * Pulse);
    }
}

void ACluePickup::FaceToLocalPlayerCamera()
{
    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC) return;

    APlayerCameraManager* CamManager = PC->PlayerCameraManager;
    if (!CamManager) return;

    const FVector CamLoc = CamManager->GetCameraLocation();
    const FVector MyLoc = ClueWidget->GetComponentLocation();

    FRotator LookAtRot = (CamLoc - MyLoc).Rotation();

    // 세워진 상태 유지, 좌우로만 회전
    LookAtRot.Pitch = 0.f;
    LookAtRot.Roll = 0.f;

    ClueWidget->SetWorldRotation(LookAtRot);
}

void ACluePickup::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!HasAuthority()) return;

    AMyCharacter* MyChar = Cast<AMyCharacter>(OtherActor);
    if (!MyChar) return;

    if (MyChar->IsEliminated()) return;

    AMyPlayerState* PS = MyChar->GetPlayerState<AMyPlayerState>();
    if (!PS) return;

    // 새로 얻었을 때만, 주운 플레이어 화면에서만 소리 재생(엔진 기본 Client RPC 사용)
    if (PS->CollectClue_Server(ClueNumber) && PickupSound)
    {
        if (APlayerController* PC = Cast<APlayerController>(MyChar->GetController()))
        {
            PC->ClientPlaySound(PickupSound, PickupSoundVolume);
        }
    }
}
