#include "CluePickup.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "MyCharacter.h"
#include "MyPlayerState.h"
#include "Components/PrimitiveComponent.h"

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
}

void ACluePickup::BeginPlay()
{
    Super::BeginPlay();
}

void ACluePickup::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    FaceToLocalPlayerCamera();
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

    PS->ServerCollectClue(ClueNumber);
}
