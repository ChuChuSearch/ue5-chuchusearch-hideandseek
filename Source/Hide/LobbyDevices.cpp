#include "LobbyDevices.h"

#include "MyPlayerController.h"
#include "MyPlayerState.h"
#include "WaitRoomGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    FLinearColor Hex(uint8 R, uint8 G, uint8 B)
    {
        return FLinearColor::FromSRGBColor(FColor(R, G, B));
    }

    const FLinearColor SeekerColor = Hex(214, 64, 70);
    const FLinearColor SeekerDark = Hex(150, 36, 46);
    const FLinearColor RunnerColor = Hex(56, 150, 96);
    const FLinearColor RunnerDark = Hex(30, 100, 60);
    const FLinearColor AnyColor = Hex(160, 140, 120);

    const AMyPlayerState* GetMyPS(const AMyPlayerController* PC)
    {
        return PC ? PC->GetPlayerState<AMyPlayerState>() : nullptr;
    }
}

// =====================================================================
// ALobbyDevice
// =====================================================================

ALobbyDevice::ALobbyDevice()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    CubeMesh = CubeFinder.Object;
    CylinderMesh = CylinderFinder.Object;
    ShapeMaterial = MaterialFinder.Object;
}

UStaticMeshComponent* ALobbyDevice::AddShape(FName Name, bool bCylinder, const FVector& Location, const FVector& SizeCm, bool bCollide)
{
    UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Comp->SetupAttachment(SceneRoot);
    Comp->SetStaticMesh(bCylinder ? CylinderMesh.Get() : CubeMesh.Get());
    Comp->SetRelativeLocation(Location);
    Comp->SetRelativeScale3D(SizeCm / 100.f);
    Comp->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
    Comp->SetCanEverAffectNavigation(bCollide);
    Comp->CastShadow = bCollide;
    Shapes.Add(Comp);
    return Comp;
}

void ALobbyDevice::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshAppearance();
    ApplyShapeColors();
}

void ALobbyDevice::BeginPlay()
{
    Super::BeginPlay();
    // 맵에서 불러온 장치는 게임 중에 OnConstruction이 다시 돌지 않으므로 여기서도 맞춘다
    RefreshAppearance();
    ApplyShapeColors();
}

void ALobbyDevice::ApplyShapeColors()
{
    for (int32 i = 0; i < Shapes.Num(); ++i)
    {
        UStaticMeshComponent* Comp = Shapes[i];
        if (!Comp || !ShapeMaterial)
        {
            continue;
        }
        UMaterialInstanceDynamic* MID = Comp->CreateDynamicMaterialInstance(0, ShapeMaterial);
        if (MID && ShapeColors.IsValidIndex(i))
        {
            MID->SetVectorParameterValue(TEXT("Color"), ShapeColors[i]);
        }
    }
}

float ALobbyDevice::GetHorizontalDistanceTo(const APawn* Pawn) const
{
    return Pawn ? FVector::Dist2D(Pawn->GetActorLocation(), GetActorLocation()) : TNumericLimits<float>::Max();
}

bool ALobbyDevice::IsPawnInRange(const APawn* Pawn) const
{
    if (!Pawn)
    {
        return false;
    }
    const float Dz = FMath::Abs(Pawn->GetActorLocation().Z - GetActorLocation().Z);
    return GetHorizontalDistanceTo(Pawn) <= InteractRadius && Dz <= 400.f;
}

// =====================================================================
// ALobbyStartPad
// =====================================================================

ALobbyStartPad::ALobbyStartPad()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.2f;

    LabelTitle = TEXT("출발 발판");
    LabelColor = Hex(236, 152, 42);
    InteractRadius = PadRadius + 40.f;
    LabelHeight = 260.f;

    // 바닥 원판 두 겹 + 가장자리 기둥 네 개(밟고 지나갈 수 있게 원판은 충돌 없음)
    AddShape(TEXT("PadOuter"), true, FVector(0.f, 0.f, 3.f), FVector(PadRadius * 2.f, PadRadius * 2.f, 6.f), false);
    AddShape(TEXT("PadInner"), true, FVector(0.f, 0.f, 5.f), FVector(PadRadius * 1.6f, PadRadius * 1.6f, 6.f), false);
    const float PostR = PadRadius + 10.f;
    int32 Index = 0;
    for (const FVector2D Dir : { FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1) })
    {
        AddShape(*FString::Printf(TEXT("Post%d"), Index++), true, FVector(Dir.X * PostR, Dir.Y * PostR, 60.f), FVector(22.f, 22.f, 120.f), true);
    }

    ShapeColors = {
        Hex(236, 152, 42),
        Hex(255, 214, 96),
        Hex(146, 92, 232), Hex(146, 92, 232), Hex(146, 92, 232), Hex(146, 92, 232),
    };
}

void ALobbyStartPad::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALobbyStartPad, CountdownEndTime);
}

bool ALobbyStartPad::IsPawnOnPad(const APawn* Pawn) const
{
    if (!Pawn)
    {
        return false;
    }
    const float Dz = Pawn->GetActorLocation().Z - GetActorLocation().Z;
    return GetHorizontalDistanceTo(Pawn) <= PadRadius && Dz > -100.f && Dz < 300.f;
}

float ALobbyStartPad::GetCountdownRemaining() const
{
    if (!IsCountingDown())
    {
        return -1.f;
    }
    const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    const float Now = GS ? GS->GetServerWorldTimeSeconds() : 0.f;
    return FMath::Max(0.f, CountdownEndTime - Now);
}

void ALobbyStartPad::CountPlayers(int32& OutPlayers, int32& OutGuests, int32& OutReadyGuests) const
{
    OutPlayers = OutGuests = OutReadyGuests = 0;
    const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    if (!GS)
    {
        return;
    }
    for (const APlayerState* PS : GS->PlayerArray)
    {
        const AMyPlayerState* MPS = Cast<AMyPlayerState>(PS);
        if (!MPS)
        {
            continue;
        }
        ++OutPlayers;
        if (!MPS->IsHost())
        {
            ++OutGuests;
            OutReadyGuests += MPS->IsReady() ? 1 : 0;
        }
    }
}

void ALobbyStartPad::BroadcastNotice(const FString& Message) const
{
    if (AWaitRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWaitRoomGameMode>() : nullptr)
    {
        GM->BroadcastLobbyChat(FString(), Message, true);
    }
}

void ALobbyStartPad::CancelCountdown(const FString& Reason)
{
    if (!IsCountingDown())
    {
        return;
    }
    CountdownEndTime = 0.f;
    CountdownHost.Reset();
    ForceNetUpdate();
    if (!Reason.IsEmpty())
    {
        BroadcastNotice(Reason);
    }
}

void ALobbyStartPad::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority())
    {
        return;
    }

    AWaitRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWaitRoomGameMode>() : nullptr;
    if (!GM)
    {
        return;
    }

    // 손님은 발판 위에 있으면 준비, 내려오면 준비 취소
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        AMyPlayerState* PS = PC ? PC->GetPlayerState<AMyPlayerState>() : nullptr;
        if (PS && !PS->IsHost())
        {
            PS->SetReady_Server(IsPawnOnPad(PC->GetPawn()));
        }
    }

    if (!IsCountingDown())
    {
        return;
    }

    // 카운트다운 중에 누가 내려가거나 방장이 바뀌면 취소
    AMyPlayerController* Host = CountdownHost.Get();
    const AMyPlayerState* HostPS = Host ? Host->GetPlayerState<AMyPlayerState>() : nullptr;
    FString FailReason;
    if (!Host || !HostPS || !HostPS->IsHost() || !IsPawnOnPad(Host->GetPawn()))
    {
        CancelCountdown(TEXT("방장이 발판에서 내려와 출발을 멈췄어요"));
        return;
    }
    if (!GM->CheckStartConditions(Host, &FailReason))
    {
        CancelCountdown(FString::Printf(TEXT("출발을 멈췄어요 (%s)"), *FailReason));
        return;
    }

    if (GetCountdownRemaining() <= 0.f)
    {
        CountdownEndTime = 0.f;
        CountdownHost.Reset();
        ForceNetUpdate();
        if (GM->TryStartGame(Host, &FailReason))
        {
            Host->ClientCloseWaitRoomUI();
        }
        else if (!FailReason.IsEmpty())
        {
            BroadcastNotice(FailReason);
        }
    }
}

void ALobbyStartPad::InteractServer(AMyPlayerController* PC)
{
    const AMyPlayerState* PS = GetMyPS(PC);
    AWaitRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWaitRoomGameMode>() : nullptr;
    if (!PS || !PS->IsHost() || !GM)
    {
        return;
    }

    if (IsCountingDown())
    {
        CancelCountdown(TEXT("방장이 출발을 취소했어요"));
        return;
    }

    if (!IsPawnOnPad(PC->GetPawn()))
    {
        PC->ClientReceiveLobbyChat(FString(), TEXT("발판 위에 올라가서 눌러 주세요"), true);
        return;
    }

    FString FailReason;
    if (!GM->CheckStartConditions(PC, &FailReason))
    {
        if (!FailReason.IsEmpty())
        {
            PC->ClientReceiveLobbyChat(FString(), FailReason, true);
        }
        return;
    }

    const AGameStateBase* GS = GetWorld()->GetGameState();
    CountdownEndTime = (GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds()) + CountdownSeconds;
    CountdownHost = PC;
    ForceNetUpdate();
    BroadcastNotice(FString::Printf(TEXT("%d초 뒤 출발해요! 발판에서 내려가면 멈춰요"), FMath::RoundToInt(CountdownSeconds)));
}

bool ALobbyStartPad::GetPrompt(const AMyPlayerController* PC, FString& OutText, bool& bOutEnabled) const
{
    const AMyPlayerState* PS = GetMyPS(PC);
    const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
    if (!PS || !Pawn)
    {
        return false;
    }

    const bool bOnPad = IsPawnOnPad(Pawn);

    if (!PS->IsHost())
    {
        if (!bOnPad)
        {
            return false;
        }
        OutText = IsCountingDown() ? TEXT("곧 출발해요!") : TEXT("준비 완료! 방장이 출발하길 기다려요");
        bOutEnabled = false;
        return true;
    }

    if (IsCountingDown())
    {
        OutText = TEXT("출발 취소");
        bOutEnabled = true;
        return true;
    }

    int32 Players = 0, Guests = 0, ReadyGuests = 0;
    CountPlayers(Players, Guests, ReadyGuests);

    if (Players < 2)
    {
        OutText = TEXT("같이 할 사람을 기다리는 중");
        bOutEnabled = false;
    }
    else if (ReadyGuests < Guests)
    {
        OutText = FString::Printf(TEXT("%d명 기다리는 중"), Guests - ReadyGuests);
        bOutEnabled = false;
    }
    else if (!bOnPad)
    {
        OutText = TEXT("발판 위에 올라가면 출발할 수 있어요");
        bOutEnabled = false;
    }
    else
    {
        OutText = TEXT("출발하기");
        bOutEnabled = true;
    }
    return true;
}

FString ALobbyStartPad::GetLabelSub(const AMyPlayerController* PC) const
{
    const float Remain = GetCountdownRemaining();
    if (Remain >= 0.f)
    {
        return FString::Printf(TEXT("%d초 뒤 출발!"), FMath::CeilToInt(Remain));
    }

    int32 Players = 0, Guests = 0, ReadyGuests = 0;
    CountPlayers(Players, Guests, ReadyGuests);
    if (Guests == 0)
    {
        return TEXT("위에 서면 준비 완료");
    }
    return FString::Printf(TEXT("준비 %d/%d · 위에 서면 준비 완료"), ReadyGuests, Guests);
}

// =====================================================================
// ALobbyRoleDoor
// =====================================================================

ALobbyRoleDoor::ALobbyRoleDoor()
{
    InteractRadius = 260.f;
    LabelHeight = 420.f;

    // 기둥 두 개 + 위 가로대 + 앞 바닥 매트
    AddShape(TEXT("PostLeft"), false, FVector(0.f, -150.f, 160.f), FVector(40.f, 40.f, 320.f), true);
    AddShape(TEXT("PostRight"), false, FVector(0.f, 150.f, 160.f), FVector(40.f, 40.f, 320.f), true);
    AddShape(TEXT("Lintel"), false, FVector(0.f, 0.f, 340.f), FVector(50.f, 360.f, 50.f), true);
    AddShape(TEXT("Mat"), false, FVector(0.f, 0.f, 2.f), FVector(220.f, 260.f, 4.f), false);
    ShapeColors = { SeekerColor, SeekerColor, SeekerDark, SeekerColor };
}

void ALobbyRoleDoor::RefreshAppearance()
{
    // 역할에 맞춰 이름과 색을 정한다
    const bool bSeeker = Preference == ERolePreference::SeekerWish;
    const bool bRunner = Preference == ERolePreference::RunnerWish;
    const FLinearColor Main = bSeeker ? SeekerColor : (bRunner ? RunnerColor : AnyColor);
    const FLinearColor Dark = bSeeker ? SeekerDark : (bRunner ? RunnerDark : AnyColor * 0.7f);
    LabelTitle = bSeeker ? TEXT("술래의 문") : (bRunner ? TEXT("러너의 문") : TEXT("상관없음의 문"));
    LabelColor = Main;
    ShapeColors = { Main, Main, Dark, Main * 0.85f };
}

bool ALobbyRoleDoor::GetPrompt(const AMyPlayerController* PC, FString& OutText, bool& bOutEnabled) const
{
    const AMyPlayerState* PS = GetMyPS(PC);
    if (!PS || !PC->GetPawn())
    {
        return false;
    }

    bOutEnabled = true;
    if (PS->GetPreference() == Preference)
    {
        OutText = TEXT("지망 취소 (상관없음)");
    }
    else
    {
        OutText = Preference == ERolePreference::SeekerWish ? TEXT("술래 지망하기")
            : (Preference == ERolePreference::RunnerWish ? TEXT("러너 지망하기") : TEXT("상관없음으로 하기"));
    }
    return true;
}

bool ALobbyRoleDoor::InteractLocal(AMyPlayerController* PC)
{
    const AMyPlayerState* PS = GetMyPS(PC);
    if (PS)
    {
        PC->RequestSetPreference(PS->GetPreference() == Preference ? ERolePreference::Any : Preference);
    }
    return false;
}

FString ALobbyRoleDoor::GetLabelSub(const AMyPlayerController* PC) const
{
    int32 Count = 0;
    if (const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr)
    {
        for (const APlayerState* PS : GS->PlayerArray)
        {
            const AMyPlayerState* MPS = Cast<AMyPlayerState>(PS);
            Count += (MPS && MPS->GetPreference() == Preference) ? 1 : 0;
        }
    }
    return FString::Printf(TEXT("지망 %d명 · E로 지망"), Count);
}

// =====================================================================
// ALobbySettingsBoard
// =====================================================================

ALobbySettingsBoard::ALobbySettingsBoard()
{
    LabelTitle = TEXT("게임 설정");
    LabelColor = Hex(146, 92, 232);
    InteractRadius = 240.f;
    LabelHeight = 330.f;

    // 다리 두 개 + 판 + 테두리
    AddShape(TEXT("LegLeft"), false, FVector(0.f, -110.f, 90.f), FVector(16.f, 16.f, 180.f), true);
    AddShape(TEXT("LegRight"), false, FVector(0.f, 110.f, 90.f), FVector(16.f, 16.f, 180.f), true);
    AddShape(TEXT("Frame"), false, FVector(0.f, 0.f, 185.f), FVector(14.f, 270.f, 160.f), true);
    AddShape(TEXT("Board"), false, FVector(-4.f, 0.f, 185.f), FVector(12.f, 240.f, 132.f), false);
    ShapeColors = { Hex(122, 74, 30), Hex(122, 74, 30), Hex(146, 92, 232), Hex(255, 248, 232) };
}

bool ALobbySettingsBoard::GetPrompt(const AMyPlayerController* PC, FString& OutText, bool& bOutEnabled) const
{
    const AMyPlayerState* PS = GetMyPS(PC);
    if (!PS || !PC->GetPawn())
    {
        return false;
    }
    OutText = PS->IsHost() ? TEXT("게임 설정 바꾸기") : TEXT("게임 설정 보기");
    bOutEnabled = true;
    return true;
}

bool ALobbySettingsBoard::InteractLocal(AMyPlayerController* PC)
{
    PC->ToggleLobbySettingsWindow();
    return false;
}

FString ALobbySettingsBoard::GetLabelSub(const AMyPlayerController* PC) const
{
    if (PC && PC->HasLobbyGameSettings())
    {
        return FString::Printf(TEXT("최대 %d명 · 방장만 바꿀 수 있어요"), PC->GetLobbyGameSettings().MaxPlayers);
    }
    return TEXT("방장만 바꿀 수 있어요");
}
