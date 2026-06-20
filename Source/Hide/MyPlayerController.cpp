#include "MyPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputCoreTypes.h"

#include "GhostCharacter.h"
#include "MyCharacter.h"
#include "MyGameMode.h"
#include "MyGameState.h"
#include "MyPlayerState.h"
#include "PasswordDoor.h"
#include "PropBase.h"
#include "WaitRoomGameMode.h"

#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "TimerManager.h"

AMyPlayerController::AMyPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    ULocalPlayer* LP = GetLocalPlayer();
    if (LP)
    {
        UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
        if (Subsystem && IMC_Default)
        {
            Subsystem->AddMappingContext(IMC_Default, 0);
        }
    }

    const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);

    if (LevelName == TEXT("WaitRoomMap"))
    {
        ShowWaitRoomUI();
        CloseGameRoomUI();
    }
    else if (LevelName == TEXT("GameMap"))
    {
        CloseWaitRoomUI();
        ShowGameRoomUI();
    }
    else
    {
        CloseWaitRoomUI();
        CloseGameRoomUI();
    }
}

void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
    if (EIC)
    {
        if (IA_Possess)
        {
            EIC->BindAction(IA_Possess, ETriggerEvent::Started, this, &AMyPlayerController::OnPossessPressed);
        }

        if (IA_Lock)
        {
            EIC->BindAction(IA_Lock, ETriggerEvent::Started, this, &AMyPlayerController::OnLockPressed);
        }

        if (IA_Attack)
        {
            EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &AMyPlayerController::OnAttackPressed);
            EIC->BindAction(IA_Attack, ETriggerEvent::Completed, this, &AMyPlayerController::OnAttackReleased);
            EIC->BindAction(IA_Attack, ETriggerEvent::Canceled, this, &AMyPlayerController::OnAttackReleased);
        }

        if (IA_GhostPlacementRefresh)
        {
            EIC->BindAction(IA_GhostPlacementRefresh, ETriggerEvent::Started, this, &AMyPlayerController::OnGhostPlacementRefreshPressed);
        }
    }
}

void AMyPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    UpdateHovered();
    UpdateGhostUI();
    UpdatePasswordInputState();

    TeamStatusRefreshElapsed += DeltaTime;
    if (TeamStatusRefreshElapsed >= TeamStatusRefreshInterval)
    {
        TeamStatusRefreshElapsed = 0.f;
        UpdateGameRoomTeamStatus();
        UpdateGameTimeWidgets();
    }

    if (AGhostCharacter* Ghost = GetGhostCharacter())
    {
        if (WasInputKeyJustPressed(EKeys::RightMouseButton) && Ghost->IsPlacementMode())
        {
            Ghost->CancelPlacementMode();
        }
    }

    if (WasInputKeyJustPressed(EKeys::RightMouseButton) && TryOpenPasswordInput())
    {
        return;
    }

    AMyCharacter* MC = GetMyCharacter();
    if (!MC) return;
    if (!MC->IsPossessing()) return;
    if (!MC->IsLocked()) return;

    float Dir = 0.f;
    if (IsInputKeyDown(EKeys::Q)) Dir -= 1.f;
    if (IsInputKeyDown(EKeys::E)) Dir += 1.f;
    if (FMath::IsNearlyZero(Dir)) return;

    MC->RequestRotateLocked(Dir);
}

void AMyPlayerController::OnPossessPressed(const FInputActionValue& Value)
{
    if (TryOpenPasswordInput())
    {
        return;
    }

    AMyCharacter* MC = GetMyCharacter();
    if (!MC) return;

    if (MC->IsPossessing())
    {
        MC->RequestReleaseProp();
        return;
    }

    APropBase* Prop = GetHoveredProp();
    if (!Prop) return;

    MC->RequestPossessProp(Prop);
}

void AMyPlayerController::OnLockPressed(const FInputActionValue& Value)
{
    AMyCharacter* MC = GetMyCharacter();
    if (!MC) return;

    MC->ToggleLocked();
}

void AMyPlayerController::UpdateHovered()
{
    AMyCharacter* MC = GetMyCharacter();

    if (MC && MC->IsSeeker())
    {
        if (OutlinedActor)
        {
            ClearOutline(OutlinedActor);
            OutlinedActor = nullptr;
        }

        HoveredActor = nullptr;
        return;
    }

    if (MC && MC->IsPossessing())
    {
        if (OutlinedActor)
        {
            ClearOutline(OutlinedActor);
            OutlinedActor = nullptr;
        }

        HoveredActor = nullptr;
        return;
    }

    FHitResult Hit;
    const bool bHit = GetScreenRayHit(Hit, bShowMouseCursor);

    AActor* NewHoveredActor = bHit ? Hit.GetActor() : nullptr;
    HoveredActor = NewHoveredActor;

    ApplyOutline(NewHoveredActor);
}

void AMyPlayerController::ApplyOutline(AActor* NewHovered)
{
    AMyCharacter* MC = GetMyCharacter();
    APawn* P = GetPawn();

    if (!MC || !P || MC->IsSeeker())
    {
        if (OutlinedActor)
        {
            ClearOutline(OutlinedActor);
            OutlinedActor = nullptr;
        }
        return;
    }

    if (!NewHovered)
    {
        if (OutlinedActor)
        {
            ClearOutline(OutlinedActor);
            OutlinedActor = nullptr;
        }
        return;
    }

    APropBase* Prop = Cast<APropBase>(NewHovered);
    if (!Prop)
    {
        if (OutlinedActor)
        {
            ClearOutline(OutlinedActor);
            OutlinedActor = nullptr;
        }
        return;
    }

    UPrimitiveComponent* Prim = Prop->GetStaticMesh();
    if (!Prim)
    {
        if (OutlinedActor)
        {
            ClearOutline(OutlinedActor);
            OutlinedActor = nullptr;
        }
        return;
    }

    int32 Stencil = 1;

    if (Prop->IsPossessionBanned())
    {
        Stencil = 3;
    }
    else if (Prop->IsPossessedNet())
    {
        Stencil = 3;
    }
    else
    {
        const float Dist = FVector::Dist(P->GetActorLocation(), Prop->GetActorLocation());

        if (Dist <= MC->MaxPossessDistance)
        {
            Stencil = 2;
        }
    }

    if (OutlinedActor && OutlinedActor != NewHovered)
    {
        ClearOutline(OutlinedActor);
        OutlinedActor = nullptr;
    }

    Prim->SetRenderCustomDepth(true);
    Prim->SetCustomDepthStencilValue(Stencil);

    OutlinedActor = NewHovered;
}

void AMyPlayerController::ClearOutline(AActor* ActorToClear)
{
    if (!ActorToClear) return;

    APropBase* Prop = Cast<APropBase>(ActorToClear);
    if (!Prop) return;

    if (UPrimitiveComponent* Prim = Prop->GetStaticMesh())
    {
        Prim->SetRenderCustomDepth(true);
        Prim->SetCustomDepthStencilValue(0);
    }
}

AMyCharacter* AMyPlayerController::GetMyCharacter() const
{
    return Cast<AMyCharacter>(GetPawn());
}

APropBase* AMyPlayerController::GetHoveredProp() const
{
    return Cast<APropBase>(HoveredActor);
}

APasswordDoor* AMyPlayerController::GetHoveredPasswordDoor() const
{
    return Cast<APasswordDoor>(HoveredActor);
}

bool AMyPlayerController::TryOpenPasswordInput()
{
    AMyCharacter* MC = GetMyCharacter();
    if (!MC || MC->IsPossessing())
    {
        return false;
    }

    APasswordDoor* Door = GetHoveredPasswordDoor();
    if (!Door)
    {
        FHitResult Hit;
        if (GetHitResultUnderCursor(ECC_Visibility, true, Hit))
        {
            Door = Cast<APasswordDoor>(Hit.GetActor());
            if (!Door && Hit.GetActor())
            {
                UE_LOG(LogTemp, Verbose, TEXT("Password input cursor hit non-door actor: %s"), *Hit.GetActor()->GetName());
            }
        }
    }

    if (!Door)
    {
        FHitResult Hit;
        if (GetScreenRayHit(Hit, false))
        {
            Door = Cast<APasswordDoor>(Hit.GetActor());
            if (!Door && Hit.GetActor())
            {
                UE_LOG(LogTemp, Verbose, TEXT("Password input center hit non-door actor: %s"), *Hit.GetActor()->GetName());
            }
        }
    }

    if (!Door)
    {
        TArray<AActor*> PasswordDoors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), APasswordDoor::StaticClass(), PasswordDoors);

        float BestDistanceSq = TNumericLimits<float>::Max();
        for (AActor* Actor : PasswordDoors)
        {
            APasswordDoor* Candidate = Cast<APasswordDoor>(Actor);
            if (!Candidate || !Candidate->CanInteractFrom(MC))
            {
                continue;
            }

            const float DistanceSq = FVector::DistSquared(Candidate->GetActorLocation(), MC->GetActorLocation());
            if (DistanceSq < BestDistanceSq)
            {
                BestDistanceSq = DistanceSq;
                Door = Candidate;
            }
        }
    }

    if (!Door || !Door->CanInteractFrom(MC))
    {
        UE_LOG(LogTemp, Verbose, TEXT("Password input failed: no interactable PasswordDoor found."));
        return false;
    }

    ActivePasswordDoor = Door;
    ShowPasswordInputUI();
    return true;
}

void AMyPlayerController::UpdatePasswordInputState()
{
    if (!IsLocalController() || !PasswordInputWidgetInstance)
    {
        return;
    }

    const AMyCharacter* MC = GetMyCharacter();
    if (!MC || !IsValid(ActivePasswordDoor) || !ActivePasswordDoor->CanInteractFrom(MC))
    {
        ClosePasswordInputUI();
        return;
    }

    double CooldownRemaining = 0.0;
    const AMyPlayerState* MyPS = GetPlayerState<AMyPlayerState>();
    const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    if (MyPS && GS)
    {
        CooldownRemaining = GS->GetCodeInputCooldownRemaining(MyPS->GetFinalRole());
    }

    BroadcastPasswordCooldownUpdated(CooldownRemaining);
}

void AMyPlayerController::BroadcastPasswordCooldownUpdated(double CooldownRemaining)
{
    if (!IsLocalController() || !PasswordInputWidgetInstance)
    {
        return;
    }

    static const FName FuncName(TEXT("OnPasswordCooldownUpdated"));
    if (UFunction* Fn = PasswordInputWidgetInstance->FindFunction(FuncName))
    {
        struct FPasswordCooldownUpdatedParams
        {
            double CooldownRemaining;
        };

        FPasswordCooldownUpdatedParams Params;
        Params.CooldownRemaining = CooldownRemaining;

        PasswordInputWidgetInstance->ProcessEvent(Fn, &Params);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Password widget has no OnPasswordCooldownUpdated event/function."));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                INDEX_NONE,
                2.0f,
                FColor::Red,
                TEXT("WBP_PasswordInput: OnPasswordCooldownUpdated not found")
            );
        }
    }
}

void AMyPlayerController::ShowWaitRoomUI()
{
    if (!IsLocalController()) return;
    if (!WBP_WaitRoom) return;
    if (WaitRoomWidgetInstance) return;

    WaitRoomWidgetInstance = CreateWidget<UUserWidget>(this, WBP_WaitRoom);
    if (!WaitRoomWidgetInstance) return;

    WaitRoomWidgetInstance->AddToViewport();

    FInputModeUIOnly Mode;
    SetInputMode(Mode);

    bShowMouseCursor = true;
}

void AMyPlayerController::UI_RequestStartGame()
{
    if (!IsLocalController()) return;
    ServerRequestStartGame();
}

void AMyPlayerController::ServerRequestStartGame_Implementation()
{
    if (!HasAuthority()) return;

    ClientCloseWaitRoomUI();

    if (AWaitRoomGameMode* GM = GetWorld()->GetAuthGameMode<AWaitRoomGameMode>())
    {
        GM->StartGame(this);
    }
}

void AMyPlayerController::ClientCloseWaitRoomUI_Implementation()
{
    CloseWaitRoomUI();
}

void AMyPlayerController::ClientEnterWaitRoomUI_Implementation()
{
    if (!IsLocalController())
    {
        return;
    }

    ClosePasswordInputUI();
    CloseGameRoomUI();

    if (GhostUIWidgetInstance)
    {
        GhostUIWidgetInstance->RemoveFromParent();
        GhostUIWidgetInstance = nullptr;
    }

    if (GameResultWidgetInstance)
    {
        GameResultWidgetInstance->RemoveFromParent();
        GameResultWidgetInstance = nullptr;
    }

    if (WaitRoomWidgetInstance)
    {
        WaitRoomWidgetInstance->RemoveFromParent();
        WaitRoomWidgetInstance = nullptr;
    }

    ShowWaitRoomUI();
}

void AMyPlayerController::CloseWaitRoomUI()
{
    if (!IsLocalController()) return;

    if (WaitRoomWidgetInstance)
    {
        WaitRoomWidgetInstance->RemoveFromParent();
        WaitRoomWidgetInstance = nullptr;
    }

    FInputModeGameOnly Mode;
    SetInputMode(Mode);
    bShowMouseCursor = false;
}

void AMyPlayerController::OnAttackPressed(const FInputActionValue& Value)
{
    if (AGhostCharacter* Ghost = GetGhostCharacter())
    {
        if (Ghost->IsPlacementMode())
        {
            Ghost->TryConfirmPlacement();
            return;
        }
    }

    bHoldTriggered = false;

    if (GetWorld())
    {
        GetWorldTimerManager().SetTimer(
            TH_AttackHold,
            this,
            &AMyPlayerController::HandleAttackHoldTriggered,
            AttackHoldThreshold,
            false
        );
    }
}

void AMyPlayerController::HandleAttackHoldTriggered()
{
    bHoldTriggered = true;

    AMyCharacter* MC = GetMyCharacter();
    if (!MC) return;

    APropBase* AimProp = GetAimPropUnderCrosshair();
    MC->RequestDestroyPropSkill(AimProp);
}

void AMyPlayerController::OnAttackReleased(const FInputActionValue& Value)
{
    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(TH_AttackHold);
    }

    if (bHoldTriggered)
    {
        return;
    }

    AMyCharacter* MC = GetMyCharacter();
    if (!MC) return;
    MC->RequestSeekerAttack();
}

APropBase* AMyPlayerController::GetAimPropUnderCrosshair() const
{
    FHitResult Hit;
    const bool bHit = GetScreenRayHit(Hit, false);
    if (!bHit) return nullptr;

    return Cast<APropBase>(Hit.GetActor());
}

bool AMyPlayerController::GetScreenRayHit(FHitResult& OutHit, bool bPreferCursor) const
{
    if (!GetWorld())
    {
        return false;
    }

    if (bPreferCursor && bShowMouseCursor)
    {
        return GetHitResultUnderCursor(ECC_Visibility, true, OutHit);
    }

    int32 ViewX = 0;
    int32 ViewY = 0;
    GetViewportSize(ViewX, ViewY);
    if (ViewX <= 0 || ViewY <= 0)
    {
        return false;
    }

    FVector WorldOrigin;
    FVector WorldDirection;
    const float ScreenX = ViewX * 0.5f;
    const float ScreenY = ViewY * 0.5f;

    if (!DeprojectScreenPositionToWorld(ScreenX, ScreenY, WorldOrigin, WorldDirection))
    {
        return false;
    }

    const FVector Start = WorldOrigin;
    const FVector End = Start + WorldDirection * 10000.f;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(ScreenRayTrace), true);
    Params.AddIgnoredActor(GetPawn());

    return GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params);
}

void AMyPlayerController::ShowGameRoomUI()
{
    if (!IsLocalController()) return;
    if (!WBP_GameRoom) return;
    if (GameRoomWidgetInstance) return;

    GameRoomWidgetInstance = CreateWidget<UUserWidget>(this, WBP_GameRoom);
    if (!GameRoomWidgetInstance) return;

    GameRoomWidgetInstance->AddToViewport();
    RefreshGameRoomCluesFromPlayerState();
    UpdateGameRoomTeamStatus();
    UpdateGameTimeWidgets();
}

void AMyPlayerController::CloseGameRoomUI()
{
    if (!IsLocalController()) return;

    if (GameRoomWidgetInstance)
    {
        GameRoomWidgetInstance->RemoveFromParent();
        GameRoomWidgetInstance = nullptr;
    }
}

void AMyPlayerController::UpdateGameRoomTeamStatus()
{
    if (!IsLocalController() || (!GameRoomWidgetInstance && !GhostUIWidgetInstance))
    {
        return;
    }

    const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    const AMyPlayerState* PS = GetPlayerState<AMyPlayerState>();
    if (!GS || !PS)
    {
        return;
    }

    int32 SeekerCount = 0;
    int32 RunnerCount = 0;
    GS->GetActiveTeamCounts(SeekerCount, RunnerCount);

    struct FUpdateTeamStatusParams
    {
        int32 SeekerCount;
        int32 RunnerCount;
        EFinalRole LocalRole;
    };

    static const FName FuncName(TEXT("UpdateTeamStatus"));
    const auto UpdateWidget = [&](UUserWidget* Widget)
    {
        if (!Widget)
        {
            return;
        }

        if (UFunction* Fn = Widget->FindFunction(FuncName))
        {
            FUpdateTeamStatusParams Params;
            Params.SeekerCount = SeekerCount;
            Params.RunnerCount = RunnerCount;
            Params.LocalRole = PS->GetFinalRole();

            Widget->ProcessEvent(Fn, &Params);
        }
    };

    UpdateWidget(GameRoomWidgetInstance);
    UpdateWidget(GhostUIWidgetInstance);
}

void AMyPlayerController::UpdateGameTimeWidgets()
{
    if (!IsLocalController())
    {
        return;
    }

    static const FName FuncName(TEXT("UpdateGameTimeUI"));
    const auto UpdateWidget = [&](UUserWidget* Widget)
    {
        if (!Widget)
        {
            return;
        }

        if (UFunction* Fn = Widget->FindFunction(FuncName))
        {
            if (Fn->ParmsSize == 0)
            {
                Widget->ProcessEvent(Fn, nullptr);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("%s.UpdateGameTimeUI must have no input parameters."), *Widget->GetName());
            }
        }
    };

    UpdateWidget(GameRoomWidgetInstance);
    UpdateWidget(GhostUIWidgetInstance);
}

void AMyPlayerController::ShowPasswordInputUI()
{
    if (!IsLocalController()) return;
    if (!WBP_PasswordInput) return;

    if (!PasswordInputWidgetInstance)
    {
        PasswordInputWidgetInstance = CreateWidget<UUserWidget>(this, WBP_PasswordInput);
        if (!PasswordInputWidgetInstance) return;

        PasswordInputWidgetInstance->AddToViewport();
    }

    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(PasswordInputWidgetInstance->TakeWidget());
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);

    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    static const FName FuncName(TEXT("OnPasswordInputOpened"));
    if (UFunction* Fn = PasswordInputWidgetInstance->FindFunction(FuncName))
    {
        struct FPasswordInputOpenedParams
        {
            TArray<int32> CollectedClueNumbers;
            double CooldownRemaining;
        };

        FPasswordInputOpenedParams Params;
        if (const AMyPlayerState* MyPS = GetPlayerState<AMyPlayerState>())
        {
            Params.CollectedClueNumbers = MyPS->GetCollectedClueNumbers();
        }

        const AMyPlayerState* MyPS = GetPlayerState<AMyPlayerState>();
        const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
        if (MyPS && GS)
        {
            Params.CooldownRemaining = GS->GetCodeInputCooldownRemaining(MyPS->GetFinalRole());
        }

        PasswordInputWidgetInstance->ProcessEvent(Fn, &Params);
    }

    UpdatePasswordInputState();
}

void AMyPlayerController::ClosePasswordInputUI()
{
    if (!IsLocalController()) return;

    if (PasswordInputWidgetInstance)
    {
        PasswordInputWidgetInstance->RemoveFromParent();
        PasswordInputWidgetInstance = nullptr;
    }

    ActivePasswordDoor = nullptr;

    FInputModeGameOnly Mode;
    SetInputMode(Mode);
    bShowMouseCursor = false;
}

AGhostCharacter* AMyPlayerController::GetGhostCharacter() const
{
    return Cast<AGhostCharacter>(GetPawn());
}

void AMyPlayerController::OnGhostPlacementRefreshPressed(const FInputActionValue& Value)
{
    AGhostCharacter* Ghost = GetGhostCharacter();
    if (!Ghost) return;

    Ghost->TryRefreshRespawnList();
}

void AMyPlayerController::UI_SelectGhostRespawnIndex(int32 InIndex)
{
    if (AGhostCharacter* Ghost = GetGhostCharacter())
    {
        Ghost->SetSelectedRespawnIndex(InIndex);

        if (!Ghost->IsPlacementMode())
        {
            Ghost->TogglePlacementMode();
        }
    }
}

void AMyPlayerController::UpdateGameRoomHearts(int32 InHitCount)
{
    if (!GameRoomWidgetInstance) return;

    static const FName FuncName(TEXT("UpdateHearts"));

    if (UFunction* Fn = GameRoomWidgetInstance->FindFunction(FuncName))
    {
        struct FUpdateHeartsParams
        {
            int32 CurrentHitCount;
        };

        FUpdateHeartsParams Params;
        Params.CurrentHitCount = InHitCount;

        GameRoomWidgetInstance->ProcessEvent(Fn, &Params);
    }
}

void AMyPlayerController::UpdateGameRoomClues(const TArray<int32>& InCollectedClueNumbers)
{
    if (!GameRoomWidgetInstance) return;

    static const FName FuncName(TEXT("UpdateClues"));

    if (UFunction* Fn = GameRoomWidgetInstance->FindFunction(FuncName))
    {
        struct FUpdateCluesParams
        {
            TArray<int32> CollectedClueNumbers;
        };

        FUpdateCluesParams Params;
        Params.CollectedClueNumbers = InCollectedClueNumbers;

        GameRoomWidgetInstance->ProcessEvent(Fn, &Params);
    }
}

void AMyPlayerController::UI_SubmitPasswordCode(const TArray<int32>& InputCode)
{
    if (!IsLocalController())
    {
        return;
    }

    ServerSubmitPasswordCode(InputCode);
}

void AMyPlayerController::UI_ClosePasswordInput()
{
    ClosePasswordInputUI();
}

void AMyPlayerController::ServerSubmitPasswordCode_Implementation(const TArray<int32>& InputCode)
{
    if (!HasAuthority())
    {
        return;
    }

    AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    if (!GS)
    {
        ClientReceivePasswordAttemptResult(0, false, 0.0);
        return;
    }

    int32 CorrectCount = 0;
    double CooldownRemaining = 0.0;
    const bool bSuccess = GS->SubmitPasswordCode(this, InputCode, CorrectCount, CooldownRemaining);
    ClientReceivePasswordAttemptResult(CorrectCount, bSuccess, CooldownRemaining);
}

void AMyPlayerController::ClientReceivePasswordAttemptResult_Implementation(int32 CorrectCount, bool bSuccess, double CooldownRemaining)
{
    if (!PasswordInputWidgetInstance)
    {
        return;
    }

    static const FName FuncName(TEXT("OnPasswordAttemptResult"));
    if (UFunction* Fn = PasswordInputWidgetInstance->FindFunction(FuncName))
    {
        struct FPasswordAttemptResultParams
        {
            int32 CorrectCount;
            bool bSuccess;
            double CooldownRemaining;
        };

        FPasswordAttemptResultParams Params;
        Params.CorrectCount = CorrectCount;
        Params.bSuccess = bSuccess;
        Params.CooldownRemaining = CooldownRemaining;

        PasswordInputWidgetInstance->ProcessEvent(Fn, &Params);
    }

    BroadcastPasswordCooldownUpdated(CooldownRemaining);
}

void AMyPlayerController::HideGameRoomUI()
{
    CloseGameRoomUI();
}

void AMyPlayerController::ShowGhostUI()
{
    if (!IsLocalController()) return;
    if (!WBP_GhostUI) return;

    if (!GhostUIWidgetInstance)
    {
        GhostUIWidgetInstance = CreateWidget<UUserWidget>(this, WBP_GhostUI);
        if (!GhostUIWidgetInstance) return;

        GhostUIWidgetInstance->AddToViewport();
    }

    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    UpdateGhostUI();
    UpdateGameRoomTeamStatus();
    UpdateGameTimeWidgets();
}

void AMyPlayerController::UpdateGhostUI()
{
    if (!IsLocalController()) return;
    if (!GhostUIWidgetInstance) return;

    AGhostCharacter* Ghost = GetGhostCharacter();
    if (!Ghost) return;

    TArray<TSubclassOf<AActor>> PropClasses;
    PropClasses.SetNum(3);

    const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    if (GS)
    {
        const TArray<FRespawnPropInfo>& RespawnList = GS->GetRespawnList();
        for (int32 Index = 0; Index < FMath::Min(RespawnList.Num(), PropClasses.Num()); ++Index)
        {
            PropClasses[Index] = RespawnList[Index].PropClass;
        }
    }

    static const FName FuncName(TEXT("UpdateGhostPlacementSlots"));
    if (UFunction* Fn = GhostUIWidgetInstance->FindFunction(FuncName))
    {
        struct FUpdateGhostPlacementSlotsParams
        {
            TArray<TSubclassOf<AActor>> VisiblePropClasses;
            int32 SelectedIndex;
            double CooldownRemaining;
            double CooldownDuration;
        };

        FUpdateGhostPlacementSlotsParams Params;
        Params.VisiblePropClasses = PropClasses;
        Params.SelectedIndex = Ghost->GetSelectedRespawnIndex();
        Params.CooldownRemaining = Ghost->GetPlacementCooldownRemaining();
        Params.CooldownDuration = Ghost->GetPlacementCooldownDuration();

        GhostUIWidgetInstance->ProcessEvent(Fn, &Params);
    }
}

void AMyPlayerController::RefreshGameRoomCluesFromPlayerState()
{
    const AMyPlayerState* MyPS = GetPlayerState<AMyPlayerState>();
    if (!MyPS)
    {
        return;
    }

    UpdateGameRoomClues(MyPS->GetCollectedClueNumbers());
}

void AMyPlayerController::OnRep_Pawn()
{
    Super::OnRep_Pawn();

    if (GetPawn() && GetPawn()->IsA(AGhostCharacter::StaticClass()))
    {
        ClientFinalizeGhostTransition();
    }
}

void AMyPlayerController::ClientFinalizeGhostTransition_Implementation()
{
    APawn* CurrentPawn = GetPawn();
    if (!CurrentPawn) return;

    SetViewTarget(CurrentPawn);

    HideGameRoomUI();
    ShowGhostUI();
}

void AMyPlayerController::ClientSetForcedSwapWarning_Implementation(bool bShowWarning, double RemainingSeconds)
{
    if (!IsLocalController())
    {
        return;
    }

    if (!GameRoomWidgetInstance)
    {
        return;
    }

    static const FName FuncName(TEXT("SetForcedSwapWarning"));
    if (UFunction* Fn = GameRoomWidgetInstance->FindFunction(FuncName))
    {
        struct FSetForcedSwapWarningParams
        {
            bool bShowWarning;
            double RemainingSeconds;
        };

        FSetForcedSwapWarningParams Params;
        Params.bShowWarning = bShowWarning;
        Params.RemainingSeconds = RemainingSeconds;

        GameRoomWidgetInstance->ProcessEvent(Fn, &Params);
    }
}

void AMyPlayerController::ClientShowGameResult_Implementation(EFinalRole WinningRole)
{
    if (!IsLocalController())
    {
        return;
    }

    const AMyPlayerState* MPS = GetPlayerState<AMyPlayerState>();
    const bool bWon = MPS && MPS->GetFinalRole() == WinningRole;

    UE_LOG(LogTemp, Log, TEXT("ClientShowGameResult: WinningRole=%d, bWon=%s, ResultWidgetClass=%s"),
        static_cast<int32>(WinningRole),
        bWon ? TEXT("true") : TEXT("false"),
        WBP_GameResult ? *WBP_GameResult->GetName() : TEXT("None"));

    if (WBP_GameResult)
    {
        if (!GameResultWidgetInstance)
        {
            GameResultWidgetInstance = CreateWidget<UUserWidget>(this, WBP_GameResult);
        }

        if (GameResultWidgetInstance)
        {
            GameResultWidgetInstance->AddToViewport(1000);

            if (UFunction* PlayResultFn = GameResultWidgetInstance->FindFunction(TEXT("PlayResult")))
            {
                struct FPlayResultParams
                {
                    EFinalRole WinningRole;
                    bool bWon;
                };

                FPlayResultParams Params;
                Params.WinningRole = WinningRole;
                Params.bWon = bWon;
                GameResultWidgetInstance->ProcessEvent(PlayResultFn, &Params);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("WBP_GameResult has no PlayResult function."));
            }

            bShowMouseCursor = true;
            FInputModeUIOnly InputMode;
            InputMode.SetWidgetToFocus(GameResultWidgetInstance->TakeWidget());
            SetInputMode(InputMode);
        }
    }

    OnGameResultReceived(WinningRole, bWon);
}
