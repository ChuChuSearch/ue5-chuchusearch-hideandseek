#include "MyPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputCoreTypes.h"

#include "GhostCharacter.h"
#include "DestroyProgressWidget.h"
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
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Components/PrimitiveComponent.h"
#include "TimerManager.h"
#include "SettingsWidget.h"
#include "GameFramework/GameUserSettings.h"
#include "HitFlashWidget.h"
#include "EliminationNoticeWidget.h"
#include "HideCameraShakes.h"
#include "ControlsGuideWidget.h"
#include "ReturnCountdownWidget.h"
#include "CrosshairWidget.h"
#include "ScoreboardWidget.h"
#include "GameTimelineBar.h"
#include "ResultScoreWidget.h"
#include "LobbyPanelWidget.h"
#include "LobbySettingsWidget.h"
#include "ReconnectWaitWidget.h"
#include "SpectateHudWidget.h"
#include "PropIconSet.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "RoleChoiceButtonWidget.h"
#include "ReadyButtonWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/Texture2D.h"
#include "NicknameLocalSubsystem.h"
#include "Components/EditableTextBox.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Misc/ConfigCacheIni.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/NavigationConfig.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "TitleWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AMyPlayerController::AMyPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    // 치트 신호기 빛기둥(패키징 시 같이 포함되도록 여기서 직접 참조)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BeaconMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (BeaconMeshFinder.Succeeded())
    {
        ClueBeaconMesh = BeaconMeshFinder.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BeaconMaterialFinder(TEXT("/Game/Game/Materials/M_CheatBeacon.M_CheatBeacon"));
    if (BeaconMaterialFinder.Succeeded())
    {
        ClueBeaconMaterial = BeaconMaterialFinder.Object;
    }

    // 유령 물체 버튼에 쓸 물체 사진 목록
    static ConstructorHelpers::FObjectFinder<UPropIconSet> PropIconsFinder(TEXT("/Game/Game/Data/DA_PropIcons.DA_PropIcons"));
    if (PropIconsFinder.Succeeded())
    {
        PropIcons = PropIconsFinder.Object;
    }
}

void AMyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DestroyClueCheatBeams();
    Super::EndPlay(EndPlayReason);
}

void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController())
    {
        LoadVolumeSettings();
        ApplyVolumeSettings();
    }

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
        PlayMusic(WaitRoomMusic);
    }
    else if (LevelName == TEXT("GameMap"))
    {
        CloseWaitRoomUI();
        ShowGameRoomUI();
    }
    else if (LevelName == TEXT("ConnectMap"))
    {
        CloseGameRoomUI();
        ShowTitleUI();
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

    // Tab은 대기실 메뉴 / 점수판 키로 쓴다. 버튼을 누른 뒤 Tab이 UI 포커스 이동에 먹히지 않게 Tab 이동을 끈다.
    if (IsLocalController() && FSlateApplication::IsInitialized())
    {
        TSharedRef<FNavigationConfig> NavConfig = MakeShared<FNavigationConfig>();
        NavConfig->bTabNavigation = false;
        FSlateApplication::Get().SetNavigationConfig(NavConfig);
    }

    // 볼륨 설정 창. 에디터 PIE에서는 ESC가 플레이 종료라서 F10도 같이 쓴다.
    if (InputComponent)
    {
        InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AMyPlayerController::ToggleSettingsMenu);
        InputComponent->BindKey(EKeys::F10, IE_Pressed, this, &AMyPlayerController::ToggleSettingsMenu);
        InputComponent->BindKey(EKeys::H, IE_Pressed, this, &AMyPlayerController::ToggleControlsGuide);
        InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AMyPlayerController::ShowScoreboard);
        InputComponent->BindKey(EKeys::Tab, IE_Released, this, &AMyPlayerController::HideScoreboard);

        // 유령 관전: V 켜기/끄기, ← → 대상 바꾸기(좌/우클릭은 OnAttackPressed / PlayerTick에서)
        InputComponent->BindKey(EKeys::V, IE_Pressed, this, &AMyPlayerController::ToggleSpectate);
        InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &AMyPlayerController::SpectateNext);
        InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &AMyPlayerController::SpectatePrev);

#if !UE_BUILD_SHIPPING
        // 치트: Ctrl+Shift+C 단서 위치/비밀번호 보기
        InputComponent->BindKey(FInputChord(EKeys::C, true, true, false, false), IE_Pressed, this, &AMyPlayerController::ToggleClueCheat);
#endif
    }
}

namespace
{
    const TCHAR* VolumeConfigSection = TEXT("/Script/Hide.VolumeSettings");
}

float AMyPlayerController::GetVolumeSetting(EVolumeChannel Channel) const
{
    switch (Channel)
    {
    case EVolumeChannel::Music: return VolumeMusic;
    case EVolumeChannel::SFX: return VolumeSFX;
    default: return VolumeMaster;
    }
}

void AMyPlayerController::SetVolumeSetting(EVolumeChannel Channel, float Value)
{
    Value = FMath::Clamp(Value, 0.f, 1.f);
    switch (Channel)
    {
    case EVolumeChannel::Music: VolumeMusic = Value; break;
    case EVolumeChannel::SFX: VolumeSFX = Value; break;
    default: VolumeMaster = Value; break;
    }
    ApplyVolumeSettings();
}

void AMyPlayerController::LoadVolumeSettings()
{
    if (!GConfig)
    {
        return;
    }

    GConfig->GetFloat(VolumeConfigSection, TEXT("Master"), VolumeMaster, GGameUserSettingsIni);
    GConfig->GetFloat(VolumeConfigSection, TEXT("Music"), VolumeMusic, GGameUserSettingsIni);
    GConfig->GetFloat(VolumeConfigSection, TEXT("SFX"), VolumeSFX, GGameUserSettingsIni);
    VolumeMaster = FMath::Clamp(VolumeMaster, 0.f, 1.f);
    VolumeMusic = FMath::Clamp(VolumeMusic, 0.f, 1.f);
    VolumeSFX = FMath::Clamp(VolumeSFX, 0.f, 1.f);

    GConfig->GetFloat(VolumeConfigSection, TEXT("MouseSensitivity"), MouseSensitivity, GGameUserSettingsIni);
    GConfig->GetBool(VolumeConfigSection, TEXT("InvertMouseY"), bInvertMouseY, GGameUserSettingsIni);
    MouseSensitivity = FMath::Clamp(MouseSensitivity, MinMouseSensitivity, MaxMouseSensitivity);
}

void AMyPlayerController::SetMouseSensitivity(float Value)
{
    MouseSensitivity = FMath::Clamp(Value, MinMouseSensitivity, MaxMouseSensitivity);
}

void AMyPlayerController::SetInvertMouseY(bool bInvert)
{
    bInvertMouseY = bInvert;
}

void AMyPlayerController::AddYawInput(float Val)
{
    Super::AddYawInput(Val * MouseSensitivity);
}

void AMyPlayerController::AddPitchInput(float Val)
{
    Super::AddPitchInput(Val * MouseSensitivity * (bInvertMouseY ? -1.f : 1.f));
}

void AMyPlayerController::SaveVolumeSettings() const
{
    if (!GConfig)
    {
        return;
    }

    GConfig->SetFloat(VolumeConfigSection, TEXT("Master"), VolumeMaster, GGameUserSettingsIni);
    GConfig->SetFloat(VolumeConfigSection, TEXT("Music"), VolumeMusic, GGameUserSettingsIni);
    GConfig->SetFloat(VolumeConfigSection, TEXT("SFX"), VolumeSFX, GGameUserSettingsIni);
    GConfig->SetFloat(VolumeConfigSection, TEXT("MouseSensitivity"), MouseSensitivity, GGameUserSettingsIni);
    GConfig->SetBool(VolumeConfigSection, TEXT("InvertMouseY"), bInvertMouseY, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

void AMyPlayerController::ApplyVolumeSettings()
{
    if (!IsLocalController() || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (!VolumeMix)
    {
        VolumeMix = NewObject<USoundMix>(this, TEXT("VolumeMix"));
        UGameplayStatics::PushSoundMixModifier(this, VolumeMix);
    }

    // 전체 볼륨은 두 클래스에 곱해서 적용
    if (USoundClass* MusicClass = MusicSoundClass.LoadSynchronous())
    {
        UGameplayStatics::SetSoundMixClassOverride(this, VolumeMix, MusicClass, VolumeMaster * VolumeMusic, 1.f, 0.f, true);
    }
    if (USoundClass* SFXClass = SFXSoundClass.LoadSynchronous())
    {
        UGameplayStatics::SetSoundMixClassOverride(this, VolumeMix, SFXClass, VolumeMaster * VolumeSFX, 1.f, 0.f, true);
    }
}

void AMyPlayerController::ClientPlayHitFlash_Implementation(bool bEliminated)
{
    if (!IsLocalController())
    {
        return;
    }

    if (!HitFlashWidget)
    {
        HitFlashWidget = CreateWidget<UHitFlashWidget>(this, UHitFlashWidget::StaticClass());
    }
    if (!HitFlashWidget)
    {
        return;
    }

    if (!HitFlashWidget->IsInViewport())
    {
        // 게임 UI 위, 설정 창(1000) 아래
        HitFlashWidget->AddToViewport(900);
    }

    HitFlashWidget->Play(bEliminated ? 1.0f : 0.8f, bEliminated ? 1.4f : 0.7f);

    // 맞은 본인 카메라만 흔든다
    if (PlayerCameraManager)
    {
        PlayerCameraManager->StartCameraShake(bEliminated
            ? UEliminatedCameraShake::StaticClass()
            : UHitCameraShake::StaticClass());
    }
}

UEliminationNoticeWidget* AMyPlayerController::EnsureNoticeWidget()
{
    if (!IsLocalController())
    {
        return nullptr;
    }

    if (!EliminationNoticeWidget)
    {
        EliminationNoticeWidget = CreateWidget<UEliminationNoticeWidget>(this, UEliminationNoticeWidget::StaticClass());
    }
    if (EliminationNoticeWidget && !EliminationNoticeWidget->IsInViewport())
    {
        // 피격 효과(900) 아래, 게임 UI 위
        EliminationNoticeWidget->AddToViewport(850);
    }
    return EliminationNoticeWidget;
}

void AMyPlayerController::ClientShowEliminationNotice_Implementation(const FString& RunnerName, int32 RemainingRunners)
{
    if (UEliminationNoticeWidget* Notice = EnsureNoticeWidget())
    {
        Notice->AddNotice(RunnerName, RemainingRunners);
    }
}

void AMyPlayerController::ClientShowConnectionNotice_Implementation(const FString& PlayerName, EConnectionNoticeKind Kind, const FString& SubText)
{
    UEliminationNoticeWidget* Notice = EnsureNoticeWidget();
    if (!Notice)
    {
        return;
    }

    auto Hex = [](uint8 R, uint8 G, uint8 B) { return FLinearColor::FromSRGBColor(FColor(R, G, B)); };
    const FString Name = PlayerName.IsEmpty() ? TEXT("플레이어") : PlayerName;

    switch (Kind)
    {
    case EConnectionNoticeKind::Disconnected:
        Notice->AddMessage(Name, TEXT(" 님의 연결이 끊겼어요"), SubText, Hex(120, 190, 255), Hex(170, 210, 255));
        break;
    case EConnectionNoticeKind::Reconnected:
        Notice->AddMessage(Name, TEXT(" 님이 다시 들어왔어요"), SubText, Hex(110, 210, 140), Hex(170, 235, 190));
        break;
    case EConnectionNoticeKind::Left:
        Notice->AddMessage(Name, TEXT(" 님이 게임을 나갔어요"), SubText, Hex(200, 190, 180), Hex(220, 210, 200));
        break;
    case EConnectionNoticeKind::DroppedOut:
        Notice->AddMessage(Name, TEXT(" 님이 돌아오지 않아 탈락했어요"), SubText, Hex(255, 128, 128), Hex(255, 160, 160));
        break;
    default:
        break;
    }
}

void AMyPlayerController::UpdateControlsGuide()
{
    if (!IsLocalController() || !GetWorld())
    {
        return;
    }

    // 게임 맵에서 게임이 진행 중일 때만 보여준다(대기실/결과 화면 제외)
    EGuideRole GuideRole = EGuideRole::None;
    const bool bGameMap = GetWorld()->GetMapName().Contains(TEXT("GameMap"));
    const AMyGameState* GS = GetWorld()->GetGameState<AMyGameState>();
    const bool bEnded = GS && GS->GetGamePhase() == EGamePhase::Ended;

    if (IsInWaitRoomMap())
    {
        // 로비 맵: 캐릭터가 생기면 로비 조작법(Tab 메뉴 안내 포함)
        if (GetPawn())
        {
            GuideRole = EGuideRole::Lobby;
        }
    }
    else if (bGameMap && !bEnded)
    {
        if (GetGhostCharacter())
        {
            GuideRole = EGuideRole::Ghost;
        }
        else if (const AMyCharacter* MC = GetMyCharacter())
        {
            GuideRole = MC->IsSeeker() ? EGuideRole::Seeker : EGuideRole::Runner;
        }
    }

    if (GuideRole == EGuideRole::None && !ControlsGuideWidget)
    {
        return;
    }

    if (!ControlsGuideWidget)
    {
        ControlsGuideWidget = CreateWidget<UControlsGuideWidget>(this, UControlsGuideWidget::StaticClass());
        if (!ControlsGuideWidget)
        {
            return;
        }
    }

    if (!ControlsGuideWidget->IsInViewport())
    {
        // 탈락 알림(850)/피격 효과(900)/설정 창(1000) 아래
        ControlsGuideWidget->AddToViewport(800);

        // 대기실 메뉴가 열려 있으면 메뉴를 가리지 않게 숨겨 둔다
        if (bLobbyMenuOpen)
        {
            ControlsGuideWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    ControlsGuideWidget->SetRole(GuideRole);
}

void AMyPlayerController::ToggleControlsGuide()
{
    if (ControlsGuideWidget)
    {
        ControlsGuideWidget->ToggleExpanded();
    }
}

void AMyPlayerController::ShowScoreboard()
{
    if (!IsLocalController() || SettingsWidget)
    {
        return;
    }

    // 대기실(로비 맵)에서 Tab은 대기실 메뉴 열기/닫기
    if (IsInWaitRoomMap())
    {
        ToggleLobbyMenu();
        return;
    }

    // 게임 맵(게임 진행 중이거나 끝난 뒤)에서만
    const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    if (!GS || GS->GetGamePhase() == EGamePhase::None)
    {
        return;
    }

    if (!ScoreboardWidget)
    {
        ScoreboardWidget = CreateWidget<UScoreboardWidget>(this, UScoreboardWidget::StaticClass());
        if (!ScoreboardWidget)
        {
            return;
        }
    }

    if (!ScoreboardWidget->IsInViewport())
    {
        // 결과 화면(1000)보다 위, 설정 창(1000)과 같은 층
        ScoreboardWidget->AddToViewport(1002);
    }
    ScoreboardWidget->RefreshNow();
}

void AMyPlayerController::ClientReceiveTeamScores_Implementation(const TArray<FTeamScoreEntry>& Entries)
{
    TeamScoreCache.Reset();
    for (const FTeamScoreEntry& Entry : Entries)
    {
        if (Entry.Player)
        {
            TeamScoreCache.Add(Entry.Player.Get(), Entry.Stats);
        }
    }
}

bool AMyPlayerController::GetVisibleScoreStats(const AMyPlayerState* PS, FHideScoreStats& OutStats) const
{
    if (!PS)
    {
        return false;
    }

    // 게임이 끝나면 모두 공개
    if (PS->AreScoresRevealed())
    {
        OutStats = PS->GetRevealedScoreStats();
        return true;
    }

    // 내 점수는 나에게만 복제된다
    if (PS == PlayerState)
    {
        OutStats = PS->GetScoreStats();
        return true;
    }

    // 팀원 점수는 서버가 보내 준 것
    if (const FHideScoreStats* Cached = TeamScoreCache.Find(PS))
    {
        OutStats = *Cached;
        return true;
    }

    return false;
}

void AMyPlayerController::HideScoreboard()
{
    if (ScoreboardWidget && ScoreboardWidget->IsInViewport())
    {
        ScoreboardWidget->RemoveFromParent();
    }
}

void AMyPlayerController::ToggleSettingsMenu()
{
    if (SettingsWidget)
    {
        CloseSettingsMenu();
        return;
    }

    if (!IsLocalController())
    {
        return;
    }

    // 설정 창이 열리면 Tab을 뗀 입력이 안 들어오므로 점수판을 먼저 닫는다
    HideScoreboard();

    SettingsWidget = CreateWidget<USettingsWidget>(this, USettingsWidget::StaticClass());
    if (!SettingsWidget)
    {
        return;
    }

    SettingsWidget->InitFromController(this);
    SettingsWidget->AddToViewport(1000);

    bSettingsPrevShowCursor = bShowMouseCursor;
    bShowMouseCursor = true;

    // 설정 창을 만지는 동안 게임 입력(클릭 공격, 이동 등)이 들어가지 않게 UI 전용 입력으로 바꾼다.
    // 누르고 있던 공격(길게 누르기)도 취소한다.
    CancelAttackInput();
    FlushPressedKeys();

    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(SettingsWidget->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
}

void AMyPlayerController::CancelAttackInput()
{
    GetWorldTimerManager().ClearTimer(TH_AttackHold);

    if (bHoldTriggered)
    {
        if (AMyCharacter* MC = GetMyCharacter())
        {
            MC->RequestCancelDestroyPropSkill();
        }
        HideDestroyProgressUI();
    }

    bHoldTriggered = false;
    AttackHoldTarget = nullptr;
}

void AMyPlayerController::LeaveToConnectScreen()
{
    if (!IsLocalController())
    {
        return;
    }

    // 설정 저장과 창 정리는 평소 닫을 때와 같은 경로로
    CloseSettingsMenu();
    StopMusic();

    // 접속 화면 맵을 열면 서버와의 연결이 끊기고(방장이면 방이 닫힌다) 새 맵의 UI가 뜬다
    UGameplayStatics::OpenLevel(this, FName(TEXT("ConnectMap")));
}

void AMyPlayerController::CloseSettingsMenu()
{
    if (!SettingsWidget)
    {
        return;
    }

    // 유지를 누르지 않은 그래픽 변경은 되돌린다
    SettingsWidget->HandleClosing();
    SettingsWidget->RemoveFromParent();
    SettingsWidget = nullptr;
    SaveVolumeSettings();

    // 설정 창에서 마우스를 떼는 순간이 게임 클릭으로 이어지지 않게
    CancelAttackInput();
    FlushPressedKeys();

    // 그래픽 설정(렌더링 배율 슬라이더는 드래그 중 저장하지 않으므로 여기서 저장)
    if (UGameUserSettings* GameSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        GameSettings->SaveSettings();
    }

    // 열기 전 상태로 되돌린다(게임 중이면 커서 숨기고 게임 입력만)
    bShowMouseCursor = bSettingsPrevShowCursor;
    if (!bSettingsPrevShowCursor)
    {
        SetInputMode(FInputModeGameOnly());
    }
    else
    {
        FInputModeGameAndUI Mode;
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        SetInputMode(Mode);
    }
}

void AMyPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    // 대기실 메뉴가 열려 있는 동안 캐릭터 이동 / 시점 막기를 유지한다
    if (bLobbyMenuOpen)
    {
        ApplyLobbyMenuInput();
    }

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

    UpdatePhaseSounds();
    UpdateControlsGuide();
    UpdateClueCheat();

    if (bSpectating)
    {
        UpdateSpectate(DeltaTime);
        if (bSpectating && WasInputKeyJustPressed(EKeys::RightMouseButton))
        {
            SpectatePrev();
        }
        return;
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

    if (bHoldTriggered && IsValid(AttackHoldTarget))
    {
        APropBase* CurrentAimProp = GetAimPropUnderCrosshair();
        if (CurrentAimProp != AttackHoldTarget)
        {
            MC->RequestCancelDestroyPropSkill();
            AttackHoldTarget = nullptr;
            HideDestroyProgressUI();
        }
    }

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
    if (bLobbyMenuOpen)
    {
        return;
    }

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

// ===== 치트: 단서 위치 / 비밀번호 보기 =====
// 단서 데이터는 서버에만 있으므로 서버에 요청해서 받아온다. Shipping 빌드에서는 아무 것도 하지 않는다.

void AMyPlayerController::ToggleClueCheat()
{
#if !UE_BUILD_SHIPPING
    if (bClueCheatOn)
    {
        bClueCheatOn = false;
        ClearClueCheatOutlines();
        DestroyClueCheatBeams();
        ClueCheatProps.Reset();
        ClueCheatCode.Reset();
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Silver, TEXT("[치트] 단서 보기 끔"));
        }
        return;
    }

    ServerRequestClueCheat();
#endif
}

void AMyPlayerController::ServerRequestClueCheat_Implementation()
{
#if !UE_BUILD_SHIPPING
    AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    if (!GS)
    {
        return;
    }

    TArray<APropBase*> ClueProps;
    TArray<int32> Code;
    GS->GetClueCheatInfo_Server(ClueProps, Code);

    UE_LOG(LogTemp, Warning, TEXT("[Cheat] %s requested clue info: %d props, code length %d"),
        *GetNameSafe(PlayerState), ClueProps.Num(), Code.Num());

    ClientReceiveClueCheat(ClueProps, Code);
#endif
}

void AMyPlayerController::ClientReceiveClueCheat_Implementation(const TArray<APropBase*>& ClueProps, const TArray<int32>& Code)
{
#if !UE_BUILD_SHIPPING
    ClearClueCheatOutlines();
    DestroyClueCheatBeams();

    ClueCheatProps.Reset();
    for (APropBase* Prop : ClueProps)
    {
        if (!Prop)
        {
            continue;
        }

        ClueCheatProps.Add(Prop);

        // 이 플레이어 화면에만 존재하는 빛기둥(복제되지 않는 로컬 컴포넌트).
        // PlayerController는 항상 숨김 액터라 그 컴포넌트는 그려지지 않으므로 프롭 액터에 붙여 만든다.
        UStaticMeshComponent* Beam = nullptr;
        if (ClueBeaconMesh && ClueBeaconMaterial)
        {
            Beam = NewObject<UStaticMeshComponent>(Prop, NAME_None, RF_Transient);
            Beam->SetStaticMesh(ClueBeaconMesh);
            Beam->SetMaterial(0, ClueBeaconMaterial);
            Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Beam->SetCastShadow(false);
            Beam->SetUsingAbsoluteLocation(true);
            Beam->SetUsingAbsoluteRotation(true);
            Beam->SetUsingAbsoluteScale(true);
            // 기본 원기둥은 지름 100, 높이 100, 중심이 가운데
            Beam->SetWorldScale3D(FVector(ClueBeaconWidth / 100.f, ClueBeaconWidth / 100.f, ClueBeaconHeight / 100.f));
            Beam->RegisterComponent();
        }
        ClueCheatBeams.Add(Beam);
    }
    ClueCheatCode = Code;
    bClueCheatOn = true;

    UpdateClueCheat();
#endif
}

void AMyPlayerController::UpdateClueCheat()
{
#if !UE_BUILD_SHIPPING
    if (!bClueCheatOn || !IsLocalController())
    {
        return;
    }

    // 마우스 올림 테두리가 지울 수 있으므로 매 프레임 다시 칠한다
    int32 Remaining = 0;
    for (int32 Index = 0; Index < ClueCheatProps.Num(); ++Index)
    {
        APropBase* Prop = ClueCheatProps[Index].Get();
        UPrimitiveComponent* Prim = Prop ? Prop->GetStaticMesh() : nullptr;
        UStaticMeshComponent* Beam = ClueCheatBeams.IsValidIndex(Index) ? ClueCheatBeams[Index].Get() : nullptr;

        if (!Prim)
        {
            // 프롭이 부서졌으면 빛기둥도 치운다(프롭과 함께 이미 사라졌을 수도 있음)
            if (IsValid(Beam))
            {
                Beam->DestroyComponent();
            }
            if (ClueCheatBeams.IsValidIndex(Index))
            {
                ClueCheatBeams[Index] = nullptr;
            }
            continue;
        }

        ++Remaining;
        Prim->SetRenderCustomDepth(true);
        Prim->SetCustomDepthStencilValue(ClueCheatStencil);

        // 프롭이 움직일 수 있으니 매 프레임 위치를 맞춘다(기둥 아래쪽 끝 = 프롭 중심)
        if (Beam)
        {
            const FVector Base = Prim->Bounds.Origin;
            Beam->SetWorldLocation(Base + FVector(0.f, 0.f, ClueBeaconHeight * 0.5f));
        }
    }

    if (GEngine)
    {
        FString CodeText;
        for (const int32 Number : ClueCheatCode)
        {
            CodeText += FString::Printf(TEXT("%d "), Number);
        }
        if (CodeText.IsEmpty())
        {
            CodeText = TEXT("(없음)");
        }

        // 같은 키로 매 프레임 덮어써서 켜져 있는 동안 계속 보이게 한다
        const uint64 MessageKey = 0x434C5545; // "CLUE"
        GEngine->AddOnScreenDebugMessage(MessageKey, 0.5f, FColor::Yellow,
            FString::Printf(TEXT("[치트] 비밀번호: %s | 남은 단서 프롭: %d개 (Ctrl+Shift+C로 끄기)"), *CodeText.TrimEnd(), Remaining));
    }
#endif
}

void AMyPlayerController::DestroyClueCheatBeams()
{
    for (UStaticMeshComponent* Beam : ClueCheatBeams)
    {
        if (IsValid(Beam))
        {
            Beam->DestroyComponent();
        }
    }
    ClueCheatBeams.Reset();
}

void AMyPlayerController::ClearClueCheatOutlines()
{
    for (const TWeakObjectPtr<APropBase>& PropPtr : ClueCheatProps)
    {
        APropBase* Prop = PropPtr.Get();
        if (!Prop || Prop == OutlinedActor)
        {
            continue;
        }

        ClearOutline(Prop);
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

    // 닉네임은 입력하는 대로 서버에 보낸다. 확인 버튼은 숨긴다(배경 연필 그림은 그대로).
    if (UEditableTextBox* NicknameBox = Cast<UEditableTextBox>(WaitRoomWidgetInstance->GetWidgetFromName(TEXT("NicknameBox"))))
    {
        NicknameBox->OnTextChanged.AddUniqueDynamic(this, &AMyPlayerController::OnNicknameTextChanged);
        NicknameBox->OnTextCommitted.AddUniqueDynamic(this, &AMyPlayerController::OnNicknameTextCommitted);

        // "닉네임을 입력하세요" / 입력 글자가 칸에 비해 커서 줄인다(WBP 기본 24).
        // 칸 높이는 옆 연필 그림 칸(43)이 정하므로 글자만 작아지고 칸 크기는 그대로다.
        NicknameBox->WidgetStyle.TextStyle.Font.Size = 18;
        NicknameBox->SynchronizeProperties();

        // 게임을 마치고 돌아온 경우: 전에 쓰던 닉네임을 채우고 서버에 다시 보낸다
        // (대기실 복귀는 재접속 방식이라 서버의 PlayerState가 새로 만들어진다)
        const UNicknameLocalSubsystem* NickSub = GetLocalPlayer() ? GetLocalPlayer()->GetSubsystem<UNicknameLocalSubsystem>() : nullptr;
        if (NickSub && !NickSub->GetNickname().IsEmpty())
        {
            NicknameBox->SetText(FText::FromString(NickSub->GetNickname()));
            PendingNickname = NickSub->GetNickname();
            LastSentNickname.Empty();
            GetWorldTimerManager().SetTimer(TH_NicknameSend, this, &AMyPlayerController::SendPendingNickname, 0.5f, false);
        }
    }
    if (UWidget* ConfirmButton = WaitRoomWidgetInstance->GetWidgetFromName(TEXT("ConfirmNicknameButton")))
    {
        ConfirmButton->SetVisibility(ESlateVisibility::Collapsed);
    }

    // 예전 글자 한 줄짜리 참가자 목록은 숨기고, 오른쪽에 참가자 목록 + 채팅 패널을 띄운다
    if (UWidget* OldList = WaitRoomWidgetInstance->GetWidgetFromName(TEXT("ScaleBox_PlayerListText")))
    {
        OldList->SetVisibility(ESlateVisibility::Collapsed);
    }
    // 역할 선택 버튼 3개(술래 지망 / 러너 지망 / 상관없음)를 같은 모양으로 넣는다.
    // 원래 WBP의 이미지 버튼 두 개는 숨기고 그 자리에 둔다(WBP 캔버스 좌표 기준).
    if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(WaitRoomWidgetInstance->GetRootWidget()))
    {
        for (const TCHAR* OldName : { TEXT("SeekerChoiceButton"), TEXT("RunnerChoiceButton") })
        {
            if (UWidget* Old = WaitRoomWidgetInstance->GetWidgetFromName(OldName))
            {
                Old->SetVisibility(ESlateVisibility::Collapsed);
            }
        }

        struct FRoleButtonSpec
        {
            ERolePreference Preference;
            const TCHAR* Label;
            FLinearColor Fill;
            FLinearColor Line;
            FVector2D Center;   // 캔버스 가운데 기준 위치
            FVector2D Size;
            int32 FontSize;
        };

        auto Hex = [](uint8 R, uint8 G, uint8 B) { return FLinearColor::FromSRGBColor(FColor(R, G, B)); };

        // 원래 버튼 자리: 술래 (-575, 27, 234x78), 러너 (-304, 29, 244x78).
        // 상관없음은 두 버튼 사이 가운데 아래(닉네임 칸 위쪽 y≈179 전까지).
        const FRoleButtonSpec Specs[] = {
            { ERolePreference::SeekerWish, TEXT("술래 지망"), Hex(232, 84, 90), Hex(176, 40, 52), FVector2D(-575.0 + 117.0, 27.0 + 39.0), FVector2D(226.0, 64.0), 24 },
            { ERolePreference::RunnerWish, TEXT("러너 지망"), Hex(72, 178, 112), Hex(36, 120, 70), FVector2D(-304.0 + 122.0, 29.0 + 39.0), FVector2D(226.0, 64.0), 24 },
            { ERolePreference::Any, TEXT("상관없음"), Hex(255, 128, 128), Hex(214, 82, 90), FVector2D((-575.0 + (-304.0 + 244.0)) * 0.5, 143.0), FVector2D(210.0, 54.0), 22 },
        };

        // 시작/준비 버튼: 원래 START 자리(같은 캔버스 배치)에 두고 START는 숨긴다. 방장에게는 "시작하기", 나머지에게는 "준비하기"
        if (UWidget* StartBox = WaitRoomWidgetInstance->GetWidgetFromName(TEXT("ScaleBox_StartButton")))
        {
            if (const UCanvasPanelSlot* StartSlot = Cast<UCanvasPanelSlot>(StartBox->Slot))
            {
                UReadyButtonWidget* ReadyButton = CreateWidget<UReadyButtonWidget>(this, UReadyButtonWidget::StaticClass());
                if (UCanvasPanelSlot* S = ReadyButton ? Canvas->AddChildToCanvas(ReadyButton) : nullptr)
                {
                    S->SetLayout(StartSlot->GetLayout());
                    S->SetAutoSize(StartSlot->GetAutoSize());
                    S->SetZOrder(StartSlot->GetZOrder());
                    ReadyButton->SetStartWidget(StartBox);
                }
            }
        }

        for (const FRoleButtonSpec& Spec : Specs)
        {
            URoleChoiceButtonWidget* RoleButton = CreateWidget<URoleChoiceButtonWidget>(this, URoleChoiceButtonWidget::StaticClass());
            if (!RoleButton)
            {
                continue;
            }

            RoleButton->Setup(Spec.Preference, Spec.Label, Spec.Fill, Spec.Line, Spec.FontSize);
            if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(RoleButton))
            {
                S->SetAnchors(FAnchors(0.5f, 0.5f));
                S->SetAlignment(FVector2D(0.5, 0.5));
                S->SetPosition(Spec.Center);
                S->SetSize(Spec.Size);
                S->SetZOrder(10);
            }
        }
    }

    if (!LobbyPanelWidget)
    {
        LobbyPanelWidget = CreateWidget<ULobbyPanelWidget>(this, ULobbyPanelWidget::StaticClass());
    }
    if (LobbyPanelWidget && !LobbyPanelWidget->IsInViewport())
    {
        LobbyPanelWidget->AddToViewport(10);
    }

    // 닉네임은 처음 화면에서 정한다. 이미 있으면 대기실 메뉴의 닉네임 칸(글 상자 + 그림)은 숨긴다.
    // (처음 화면을 거치지 않고 대기실로 바로 들어온 경우에만 여기서 입력)
    const UNicknameLocalSubsystem* NickSub = GetLocalPlayer() ? GetLocalPlayer()->GetSubsystem<UNicknameLocalSubsystem>() : nullptr;
    const bool bNeedsNickname = !NickSub || NickSub->GetNickname().IsEmpty();
    if (!bNeedsNickname)
    {
        if (UWidget* NicknameRow = WaitRoomWidgetInstance->GetWidgetFromName(TEXT("ScaleBox_Nickname")))
        {
            NicknameRow->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // 로비 맵이 보이도록 화면 전체를 덮는 대기실 배경 그림은 숨긴다(메뉴는 맵 위에 뜬다)
    if (WaitRoomWidgetInstance->WidgetTree)
    {
        WaitRoomWidgetInstance->WidgetTree->ForEachWidget([bNeedsNickname](UWidget* Widget)
        {
            const UImage* Image = Cast<UImage>(Widget);
            const UObject* Resource = Image ? Image->GetBrush().GetResourceObject() : nullptr;
            if (!Resource)
            {
                return;
            }
            const FString ResName = Resource->GetName();
            if (ResName.Contains(TEXT("Mainscreen_Background")) || (!bNeedsNickname && ResName.Contains(TEXT("NicknameInput"))))
            {
                Widget->SetVisibility(ESlateVisibility::Collapsed);
            }
        });
    }

    // 닉네임이 아직 없으면 메뉴를 열어 두고, 있으면 바로 맵을 돌아다닌다
    bLobbyMenuOpen = !bNeedsNickname; // SetLobbyMenuOpen이 바뀐 경우에만 처리하므로 반대값에서 시작
    SetLobbyMenuOpen(bNeedsNickname);
}

void AMyPlayerController::ShowTitleUI()
{
    if (!IsLocalController())
    {
        return;
    }

    if (!TitleWidget)
    {
        TitleWidget = CreateWidget<UTitleWidget>(this, UTitleWidget::StaticClass());
    }
    if (TitleWidget && !TitleWidget->IsInViewport())
    {
        TitleWidget->AddToViewport(5);
    }

    RemoveLegacyConnectWidget();
    // ConnectMap 레벨 블루프린트가 예전 접속 화면(WBP_ServerConnect)을 같은 프레임에 띄우므로 조금 뒤에 한 번 더 치운다
    GetWorldTimerManager().SetTimer(TH_TitleCleanup, this, &AMyPlayerController::RemoveLegacyConnectWidget, 0.2f, false);
}

void AMyPlayerController::RemoveLegacyConnectWidget()
{
    TArray<UUserWidget*> Found;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Found, UUserWidget::StaticClass(), true);
    for (UUserWidget* Widget : Found)
    {
        if (Widget && Widget->GetClass()->GetName().StartsWith(TEXT("WBP_ServerConnect")))
        {
            Widget->RemoveFromParent();
        }
    }

    if (!TitleWidget)
    {
        return;
    }

    FInputModeUIOnly Mode;
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    if (UWidget* Focus = TitleWidget->GetInitialFocusWidget())
    {
        Mode.SetWidgetToFocus(Focus->TakeWidget());
    }
    SetInputMode(Mode);
    SetShowMouseCursor(true);
}

bool AMyPlayerController::IsInWaitRoomMap() const
{
    return UGameplayStatics::GetCurrentLevelName(this, true) == TEXT("WaitRoomMap");
}

void AMyPlayerController::ToggleLobbyMenu()
{
    if (!IsLocalController() || !WaitRoomWidgetInstance)
    {
        return;
    }

    SetLobbyMenuOpen(!bLobbyMenuOpen);
}

void AMyPlayerController::SetLobbyMenuOpen(bool bOpen)
{
    if (!IsLocalController() || bLobbyMenuOpen == bOpen)
    {
        return;
    }

    bLobbyMenuOpen = bOpen;

    // 닫혀 있어도 채팅이 쌓이도록 위젯은 지우지 않고 숨기기만 한다
    const ESlateVisibility MenuVisibility = bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
    if (WaitRoomWidgetInstance)
    {
        WaitRoomWidgetInstance->SetVisibility(MenuVisibility);
    }
    if (LobbyPanelWidget)
    {
        LobbyPanelWidget->SetVisibility(MenuVisibility);
    }
    if (!bOpen && LobbySettingsWidget)
    {
        LobbySettingsWidget->RemoveFromParent();
        LobbySettingsWidget = nullptr;
    }
    if (ControlsGuideWidget)
    {
        ControlsGuideWidget->SetVisibility(bOpen ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
    }

    // 메뉴를 여닫는 순간 누르고 있던 키가 캐릭터에 남지 않게
    FlushPressedKeys();

    if (bOpen)
    {
        // UI 전용 입력이면 Tab / ESC / F10이 플레이어 컨트롤러까지 오지 않으므로 게임+UI 겸용.
        // 대신 캐릭터 이동과 시점 회전은 막는다(글 상자에 쓰는 글자는 글 상자가 먼저 받는다).
        FInputModeGameAndUI Mode;
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        SetInputMode(Mode);
        bShowMouseCursor = true;
    }
    else
    {
        // 글 상자에 남은 키보드 포커스를 게임 화면으로 돌려준다
        FSlateApplication::Get().SetAllUserFocusToGameViewport();
        SetInputMode(FInputModeGameOnly());
        bShowMouseCursor = false;
    }

    ApplyLobbyMenuInput();
}

void AMyPlayerController::ApplyLobbyMenuInput()
{
    // 이동 / 시점 무시는 횟수로 쌓이므로 상태가 다를 때만 바꾼다
    // (캐릭터를 새로 조종하면 엔진이 무시 상태를 초기화하므로 PlayerTick에서도 맞춘다)
    const bool bBlock = bLobbyMenuOpen && !SettingsWidget;
    if (bBlock)
    {
        if (!IsMoveInputIgnored())
        {
            SetIgnoreMoveInput(true);
        }
        if (!IsLookInputIgnored())
        {
            SetIgnoreLookInput(true);
        }
    }
    else
    {
        ResetIgnoreMoveInput();
        ResetIgnoreLookInput();
    }
}

void AMyPlayerController::OnNicknameTextChanged(const FText& Text)
{
    PendingNickname = Text.ToString();

    // 글자마다 보내지 않고, 입력이 잠깐 멈추면 한 번 보낸다
    GetWorldTimerManager().SetTimer(TH_NicknameSend, this, &AMyPlayerController::SendPendingNickname, 0.35f, false);
}

void AMyPlayerController::OnNicknameTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    // Enter를 누르거나 다른 곳을 클릭하면 바로 보낸다
    PendingNickname = Text.ToString();
    GetWorldTimerManager().ClearTimer(TH_NicknameSend);
    SendPendingNickname();
}

void AMyPlayerController::SendPendingNickname()
{
    const FString Nickname = PendingNickname.TrimStartAndEnd();
    if (Nickname.IsEmpty() || Nickname == LastSentNickname)
    {
        return;
    }

    AMyPlayerState* PS = GetPlayerState<AMyPlayerState>();
    if (!PS)
    {
        // 접속 직후라 아직 PlayerState가 안 왔으면 잠시 뒤 다시 시도
        GetWorldTimerManager().SetTimer(TH_NicknameSend, this, &AMyPlayerController::SendPendingNickname, 0.5f, false);
        return;
    }

    PS->ServerSetNickname(Nickname);
    LastSentNickname = Nickname;

    if (UNicknameLocalSubsystem* NickSub = GetLocalPlayer() ? GetLocalPlayer()->GetSubsystem<UNicknameLocalSubsystem>() : nullptr)
    {
        NickSub->SetNickname(Nickname);
    }
}

void AMyPlayerController::UI_RequestStartGame()
{
    if (!IsLocalController()) return;

    // 입력 직후 바로 시작을 눌러도 대기 중인 닉네임을 먼저 보낸다
    if (GetWorldTimerManager().IsTimerActive(TH_NicknameSend))
    {
        GetWorldTimerManager().ClearTimer(TH_NicknameSend);
        SendPendingNickname();
    }

    ServerRequestStartGame();
}

void AMyPlayerController::ServerRequestStartGame_Implementation()
{
    if (!HasAuthority()) return;

    // 방장이 아니거나 인원이 맞지 않으면 시작되지 않으므로, 실제로 시작될 때만 UI를 닫는다.
    AWaitRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWaitRoomGameMode>() : nullptr;
    FString FailReason;
    if (GM && GM->TryStartGame(this, &FailReason))
    {
        ClientCloseWaitRoomUI();
    }
    else if (!FailReason.IsEmpty())
    {
        // 왜 시작이 안 됐는지 누른 사람 채팅 칸에 안내
        ClientReceiveLobbyChat(FString(), FailReason, true);
    }
}

void AMyPlayerController::RequestToggleReady()
{
    if (AMyPlayerState* PS = GetPlayerState<AMyPlayerState>())
    {
        PS->ServerSetReady(!PS->IsReady());
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

    StopFeverLoop();
    PlayMusic(WaitRoomMusic);
}

void AMyPlayerController::SendLobbyChat(const FString& Message)
{
    const FString Trimmed = Message.TrimStartAndEnd();
    if (!Trimmed.IsEmpty())
    {
        ServerSendLobbyChat(Trimmed.Left(AWaitRoomGameMode::MaxChatLength));
    }
}

void AMyPlayerController::ServerSendLobbyChat_Implementation(const FString& Message)
{
    // 대기실에서만(게임 중에는 대기실 게임 모드가 없어서 무시된다)
    if (AWaitRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWaitRoomGameMode>() : nullptr)
    {
        GM->HandleLobbyChat(this, Message);
    }
}

void AMyPlayerController::ClientSetLobbyGameSettings_Implementation(const FLobbyGameSettings& InSettings)
{
    LobbyGameSettings = InSettings;
    bHasLobbyGameSettings = true;
}

void AMyPlayerController::ServerSetLobbyGameSettings_Implementation(const FLobbyGameSettings& InSettings, bool bResetToDefaults)
{
    if (AWaitRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWaitRoomGameMode>() : nullptr)
    {
        GM->HandleSetLobbyGameSettings(this, InSettings, bResetToDefaults);
    }
}

void AMyPlayerController::ToggleLobbySettingsWindow()
{
    if (!IsLocalController())
    {
        return;
    }

    if (LobbySettingsWidget)
    {
        LobbySettingsWidget->RemoveFromParent();
        LobbySettingsWidget = nullptr;
        return;
    }

    LobbySettingsWidget = CreateWidget<ULobbySettingsWidget>(this, ULobbySettingsWidget::StaticClass());
    if (LobbySettingsWidget)
    {
        // 대기실 화면(10) 위, 설정 창(1000) 아래
        LobbySettingsWidget->AddToViewport(500);
    }
}

void AMyPlayerController::RequestSetPreference(ERolePreference InPreference)
{
    if (AMyPlayerState* PS = GetPlayerState<AMyPlayerState>())
    {
        PS->ServerSetPreference(InPreference);
    }
}

void AMyPlayerController::ClientReceiveLobbyChat_Implementation(const FString& SenderName, const FString& Message, bool bSystem)
{
    if (LobbyPanelWidget)
    {
        LobbyPanelWidget->AddChatLine(SenderName, Message, bSystem);
    }
}

void AMyPlayerController::CloseWaitRoomUI()
{
    if (!IsLocalController()) return;

    if (WaitRoomWidgetInstance)
    {
        WaitRoomWidgetInstance->RemoveFromParent();
        WaitRoomWidgetInstance = nullptr;
    }

    if (LobbyPanelWidget)
    {
        LobbyPanelWidget->RemoveFromParent();
        LobbyPanelWidget = nullptr;
    }

    if (LobbySettingsWidget)
    {
        LobbySettingsWidget->RemoveFromParent();
        LobbySettingsWidget = nullptr;
    }

    // 대기실 메뉴 때문에 막아 둔 이동 / 시점을 풀어 둔다
    if (bLobbyMenuOpen)
    {
        bLobbyMenuOpen = false;
        ResetIgnoreMoveInput();
        ResetIgnoreLookInput();
    }
    if (ControlsGuideWidget)
    {
        ControlsGuideWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    FInputModeGameOnly Mode;
    SetInputMode(Mode);
    bShowMouseCursor = false;
}

void AMyPlayerController::OnAttackPressed(const FInputActionValue& Value)
{
    // 설정 창 / 대기실 메뉴가 열려 있으면 클릭은 UI 조작이다(투사체 발사 금지)
    if (SettingsWidget || bLobbyMenuOpen)
    {
        return;
    }

    // 관전 중 좌클릭은 다음 대상
    if (bSpectating)
    {
        SpectateNext();
        return;
    }

    if (AGhostCharacter* Ghost = GetGhostCharacter())
    {
        if (Ghost->IsPlacementMode())
        {
            Ghost->TryConfirmPlacement();
            return;
        }
    }

    bHoldTriggered = false;
    AttackHoldTarget = nullptr;

    if (AMyCharacter* MC = GetMyCharacter())
    {
        if (MC->IsPossessing())
        {
            return;
        }
    }

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
    if (MC->IsPossessing()) return;

    APropBase* AimProp = GetAimPropUnderCrosshair();
    AttackHoldTarget = AimProp;
    MC->RequestDestroyPropSkill(AimProp);
}

void AMyPlayerController::OnAttackReleased(const FInputActionValue& Value)
{
    if (SettingsWidget)
    {
        CancelAttackInput();
        return;
    }

    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(TH_AttackHold);
    }

    if (bHoldTriggered)
    {
        if (AMyCharacter* MC = GetMyCharacter())
        {
            MC->RequestCancelDestroyPropSkill();
        }

        AttackHoldTarget = nullptr;
        bHoldTriggered = false;
        HideDestroyProgressUI();
        return;
    }

    AMyCharacter* MC = GetMyCharacter();
    if (!MC) return;
    if (MC->RequestSeekerAttack() && CrosshairWidget)
    {
        CrosshairWidget->NotifyFired();
    }
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

    // WBP_GameRoom에 있던 예전 조준점 이미지는 숨기고 C++로 그린 조준점을 대신 띄운다
    if (UWidget* OldCrosshair = GameRoomWidgetInstance->GetWidgetFromName(TEXT("Crosshair")))
    {
        OldCrosshair->SetVisibility(ESlateVisibility::Collapsed);
    }
    AttachTimelineBar();

    // 한 팀이 모두 끊겼을 때의 재접속 카운트다운 띠(평소에는 숨어 있다)
    if (!ReconnectWaitWidget)
    {
        ReconnectWaitWidget = CreateWidget<UReconnectWaitWidget>(this, UReconnectWaitWidget::StaticClass());
        if (ReconnectWaitWidget)
        {
            ReconnectWaitWidget->AddToViewport(860);
        }
    }

    if (!CrosshairWidget)
    {
        CrosshairWidget = CreateWidget<UCrosshairWidget>(this, UCrosshairWidget::StaticClass());
        if (CrosshairWidget)
        {
            CrosshairWidget->AddToViewport(5);
        }
    }

    RefreshGameRoomCluesFromPlayerState();
    UpdateGameRoomTeamStatus();
    UpdateGameTimeWidgets();
}

void AMyPlayerController::AttachTimelineBar()
{
    if (!GameRoomWidgetInstance)
    {
        return;
    }

    // WBP_GameRoom 위쪽 프레임: Scale_TimeBar(ScaleBox) > Overlay > TimeBar(프레임 이미지) + 팀 이름 + 가운데 시간.
    // 프레임 이미지 바로 위에 타임라인 바를 넣고, 이미지 안 빈 홈 위치만큼 여백을 줘서 홈에 딱 맞춘다.
    UImage* Frame = Cast<UImage>(GameRoomWidgetInstance->GetWidgetFromName(TEXT("TimeBar")));
    UOverlay* FrameOverlay = Frame ? Cast<UOverlay>(Frame->GetParent()) : nullptr;
    if (!FrameOverlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("Timeline bar: TimeBar image or its Overlay parent not found in WBP_GameRoom."));
        return;
    }

    FVector2D FrameSize = Frame->GetBrush().ImageSize;
    if (const UTexture2D* Tex = Cast<UTexture2D>(Frame->GetBrush().GetResourceObject()))
    {
        if (FrameSize.X <= 1.0 || FrameSize.Y <= 1.0)
        {
            FrameSize = FVector2D(Tex->GetSizeX(), Tex->GetSizeY());
        }
    }
    if (FrameSize.X <= 1.0 || FrameSize.Y <= 1.0)
    {
        return;
    }

    FMargin FramePadding;
    if (const UOverlaySlot* FrameSlot = Cast<UOverlaySlot>(Frame->Slot))
    {
        FramePadding = FrameSlot->GetPadding();
    }

    UGameTimelineBar* Bar = CreateWidget<UGameTimelineBar>(this, UGameTimelineBar::StaticClass());
    if (!Bar)
    {
        return;
    }

    const int32 FrameIndex = FrameOverlay->GetChildIndex(Frame);
    if (UOverlaySlot* BarSlot = Cast<UOverlaySlot>(FrameOverlay->InsertChildAt(FrameIndex + 1, Bar)))
    {
        BarSlot->SetHorizontalAlignment(HAlign_Fill);
        BarSlot->SetVerticalAlignment(VAlign_Fill);
        BarSlot->SetPadding(FMargin(
            FramePadding.Left + FrameSize.X * UGameTimelineBar::GrooveLeft,
            FramePadding.Top + FrameSize.Y * UGameTimelineBar::GrooveTop,
            FramePadding.Right + FrameSize.X * (1.f - UGameTimelineBar::GrooveRight),
            FramePadding.Bottom + FrameSize.Y * (1.f - UGameTimelineBar::GrooveBottom)));
    }
}

void AMyPlayerController::CloseGameRoomUI()
{
    if (!IsLocalController()) return;

    HideDestroyProgressUI();

    if (GameRoomWidgetInstance)
    {
        GameRoomWidgetInstance->RemoveFromParent();
        GameRoomWidgetInstance = nullptr;
    }

    if (CrosshairWidget)
    {
        CrosshairWidget->RemoveFromParent();
        CrosshairWidget = nullptr;
    }

    if (ReconnectWaitWidget)
    {
        ReconnectWaitWidget->RemoveFromParent();
        ReconnectWaitWidget = nullptr;
    }
}

void AMyPlayerController::ShowDestroyProgressUI(AMyCharacter* SourceCharacter)
{
    if (!IsLocalController() || !WBP_DestroyProgress || !IsValid(SourceCharacter))
    {
        return;
    }

    if (!DestroyProgressWidgetInstance)
    {
        DestroyProgressWidgetInstance = CreateWidget<UDestroyProgressWidget>(this, WBP_DestroyProgress);
        if (!DestroyProgressWidgetInstance)
        {
            return;
        }

        DestroyProgressWidgetInstance->AddToViewport(50);
    }

    DestroyProgressWidgetInstance->BeginDestroyProgress(SourceCharacter);
}

void AMyPlayerController::HideDestroyProgressUI()
{
    if (!IsLocalController())
    {
        return;
    }

    if (DestroyProgressWidgetInstance)
    {
        DestroyProgressWidgetInstance->EndDestroyProgress();
        DestroyProgressWidgetInstance->RemoveFromParent();
        DestroyProgressWidgetInstance = nullptr;
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
    // 같은 R키: 유령이면 리스폰 목록 새로고침, 프롭 빙의 중이면 프롭 각도 초기화
    if (AGhostCharacter* Ghost = GetGhostCharacter())
    {
        Ghost->TryRefreshRespawnList();
        return;
    }

    if (AMyCharacter* MC = GetMyCharacter())
    {
        if (MC->IsPossessing())
        {
            MC->RequestResetPropRotation();
        }
    }
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

void AMyPlayerController::UpdateGameRoomClues(const TArray<int32>& InPositionedClueNumbers)
{
    if (!GameRoomWidgetInstance) return;

    // 자리 배치는 서버가 계산해 PlayerState로 보내준다(클라이언트는 정답 순서를 모름).
    // 아직 수집한 단서가 없으면 빈 칸(INDEX_NONE)으로 채워 위젯이 항상 같은 길이를 받게 한다.
    TArray<int32> PositionedClues = InPositionedClueNumbers;
    if (const AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr)
    {
        while (PositionedClues.Num() < GS->GetPasswordCodeLength())
        {
            PositionedClues.Add(INDEX_NONE);
        }
    }

    static const FName FuncName(TEXT("UpdateClues"));

    if (UFunction* Fn = GameRoomWidgetInstance->FindFunction(FuncName))
    {
        struct FUpdateCluesParams
        {
            TArray<int32> CollectedClueNumbers;
        };

        FUpdateCluesParams Params;
        Params.CollectedClueNumbers = PositionedClues;

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

    if (!CanSubmitPasswordCode_Server(GS))
    {
        UE_LOG(LogTemp, Warning, TEXT("Rejected password submit from %s: not eligible (pawn/phase/distance)."), *GetNameSafe(this));
        ClientReceivePasswordAttemptResult(0, false, 0.0);
        return;
    }

    int32 CorrectCount = 0;
    double CooldownRemaining = 0.0;
    const bool bSuccess = GS->SubmitPasswordCode(this, InputCode, CorrectCount, CooldownRemaining);
    ClientReceivePasswordAttemptResult(CorrectCount, bSuccess, CooldownRemaining);
}

bool AMyPlayerController::CanSubmitPasswordCode_Server(const AMyGameState* GS) const
{
    if (!HasAuthority() || !GS || !GetWorld())
    {
        return false;
    }

    if (GS->GetGamePhase() == EGamePhase::Ended)
    {
        return false;
    }

    // 유령(AGhostCharacter)이나 탈락자는 제출 불가.
    const AMyCharacter* MC = GetMyCharacter();
    if (!MC || MC->IsEliminated())
    {
        return false;
    }

    // 문 근처인지 서버에서 다시 확인(클라이언트 판정만 믿지 않음). 지연 보정으로 약간의 여유를 둔다.
    TArray<AActor*> PasswordDoors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APasswordDoor::StaticClass(), PasswordDoors);
    for (const AActor* Actor : PasswordDoors)
    {
        const APasswordDoor* Door = Cast<APasswordDoor>(Actor);
        if (Door && Door->CanInteractFrom(MC, PasswordSubmitDistanceTolerance))
        {
            return true;
        }
    }

    return false;
}

void AMyPlayerController::ClientReceivePasswordAttemptResult_Implementation(int32 CorrectCount, bool bSuccess, double CooldownRemaining)
{
    PlayUISound(bSuccess ? PasswordCorrectSound : PasswordWrongSound);

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

    ApplyGhostSlotIcons(PropClasses);
}

void AMyPlayerController::ApplyGhostSlotIcons(const TArray<TSubclassOf<AActor>>& PropClasses)
{
    if (!GhostUIWidgetInstance)
    {
        return;
    }

    // WBP_GhostUI 왼쪽 아래 물체 버튼 세 개(Object_0~2)의 그림 칸(ObjectImage_0~2)에 물체 사진을 넣는다.
    // 사진은 PropIconCommandlet으로 미리 찍어 둔 것(DA_PropIcons).
    for (int32 Index = 0; Index < PropClasses.Num(); ++Index)
    {
        UImage* Image = Cast<UImage>(GhostUIWidgetInstance->GetWidgetFromName(*FString::Printf(TEXT("ObjectImage_%d"), Index)));
        if (!Image)
        {
            continue;
        }

        UTexture2D* Icon = (PropIcons && PropClasses[Index]) ? PropIcons->FindIcon(PropClasses[Index]) : nullptr;
        if (Image->GetBrush().GetResourceObject() == Icon && (Icon != nullptr) == Image->IsVisible())
        {
            continue;
        }

        if (!Icon)
        {
            // 빈 칸이면 그림을 숨긴다
            Image->SetVisibility(ESlateVisibility::Hidden);
            continue;
        }

        // 버튼 크기(SizeBox)에 맞춰 여백을 조금 두고 꽉 차게
        float Side = 110.f;
        if (const USizeBox* Box = Cast<USizeBox>(GhostUIWidgetInstance->GetWidgetFromName(*FString::Printf(TEXT("SizeBox_%d"), Index))))
        {
            const float W = Box->GetWidthOverride();
            const float H = Box->GetHeightOverride();
            if (W > 1.f && H > 1.f)
            {
                Side = FMath::Min(W, H) * 0.86f;
            }
        }

        FSlateBrush Brush = Image->GetBrush();
        Brush.SetResourceObject(Icon);
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.ImageSize = FVector2D(Side, Side);
        Brush.TintColor = FSlateColor(FLinearColor::White);
        Image->SetBrush(Brush);
        Image->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void AMyPlayerController::RefreshGameRoomCluesFromPlayerState()
{
    const AMyPlayerState* MyPS = GetPlayerState<AMyPlayerState>();
    if (!MyPS)
    {
        return;
    }

    UpdateGameRoomClues(MyPS->GetPositionedClueNumbers());
}

void AMyPlayerController::OnRep_Pawn()
{
    Super::OnRep_Pawn();

    if (GetPawn() && GetPawn()->IsA(AGhostCharacter::StaticClass()))
    {
        ClientFinalizeGhostTransition();
    }
}

void AMyPlayerController::PawnLeavingGame()
{
    AMyGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMyGameMode>() : nullptr;
    if (GM && GM->ShouldKeepPawnOnDisconnect() && GetPawn())
    {
        // 기본 동작(캐릭터 파괴) 대신 조종만 해제하고 캐릭터는 월드에 남긴다.
        UnPossess();
        return;
    }

    Super::PawnLeavingGame();
}

void AMyPlayerController::ClientRestoreAfterReconnect_Implementation(int32 SeekerHitCount)
{
    if (!IsLocalController())
    {
        return;
    }

    ShowGameRoomUI();
    UpdateGameRoomHearts(SeekerHitCount);
    RefreshGameRoomCluesFromPlayerState();
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
    ShowGameResultLocal(WinningRole);
}

void AMyPlayerController::ShowGameResultLocal(EFinalRole WinningRole)
{
    if (!IsLocalController())
    {
        return;
    }

    // 플레이 종료/맵 이동으로 월드를 정리하는 중이면 결과 화면을 만들지 않는다
    if (!GetWorld() || GetWorld()->bIsTearingDown)
    {
        return;
    }

    AMyGameState* ResultSource = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    if (GameResultWidgetInstance && LastGameResultSource.Get() == ResultSource)
    {
        return;
    }

    LastGameResultSource = ResultSource;

    // 관전 중이었으면 끝내고 결과 화면으로
    StopSpectate();

    const AMyPlayerState* MPS = GetPlayerState<AMyPlayerState>();
    const bool bWon = MPS && MPS->GetFinalRole() == WinningRole;

    StopFeverLoop();
    StopMusic();
    PlayUISound(bWon ? WinSound : LoseSound);

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

    // 결과 화면 위에 대기실 복귀 카운트다운(+ 방장용 바로 이동 버튼)
    if (!ReturnCountdownWidget)
    {
        ReturnCountdownWidget = CreateWidget<UReturnCountdownWidget>(this, UReturnCountdownWidget::StaticClass());
    }
    if (ReturnCountdownWidget && !ReturnCountdownWidget->IsInViewport())
    {
        ReturnCountdownWidget->AddToViewport(1001);
        bShowMouseCursor = true;
    }

    // 결과 영상 위 오른쪽에 이번 판 점수(MVP, 팀별 순위, 칭호). 결과 화면은 UI 전용 입력이라 Tab 점수판이 안 열린다.
    HideScoreboard();
    if (!ResultScoreWidget)
    {
        ResultScoreWidget = CreateWidget<UResultScoreWidget>(this, UResultScoreWidget::StaticClass());
    }
    if (ResultScoreWidget && !ResultScoreWidget->IsInViewport())
    {
        ResultScoreWidget->SetWinningRole(WinningRole);
        ResultScoreWidget->AddToViewport(1001);
    }

    OnGameResultReceived(WinningRole, bWon);
}

void AMyPlayerController::RequestReturnToWaitRoomNow()
{
    ServerRequestReturnToWaitRoom();
}

void AMyPlayerController::ServerRequestReturnToWaitRoom_Implementation()
{
    if (AMyGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMyGameMode>() : nullptr)
    {
        GM->RequestReturnToWaitRoom(this);
    }
}

// ===== 페이즈 소리 / 배경음 =====
// 모두 로컬 화면용이라 RPC 없이 각 클라가 복제된 GameState의 페이즈를 보고 직접 재생한다.

void AMyPlayerController::UpdatePhaseSounds()
{
    if (!IsLocalController())
    {
        return;
    }

    AMyGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
    if (!GS)
    {
        return;
    }

    // 맵이 바뀌어 GameState가 새로 생기면 처음부터 다시 본다.
    if (LastSoundGameState.Get() != GS)
    {
        LastSoundGameState = GS;
        LastSoundPhase = EGamePhase::None;
        LastTickSecond = -1;
    }

    const EGamePhase Phase = GS->GetGamePhase();
    if (Phase != LastSoundPhase)
    {
        const EGamePhase OldPhase = LastSoundPhase;
        LastSoundPhase = Phase;
        LastTickSecond = -1;
        HandlePhaseChangedLocal(OldPhase, Phase);
    }

    // 경기 종료 직전(피버 마지막 N초) 1초마다 똑딱
    if (Phase == EGamePhase::FeverTime && LastSecondsTickSound && LastSecondsTickCount > 0)
    {
        const int32 Second = FMath::CeilToInt(static_cast<float>(GS->GetPhaseRemainingSeconds()));
        if (Second > 0 && Second <= LastSecondsTickCount && Second != LastTickSecond)
        {
            LastTickSecond = Second;
            PlayUISound(LastSecondsTickSound);
        }
    }
}

void AMyPlayerController::HandlePhaseChangedLocal(EGamePhase OldPhase, EGamePhase NewPhase)
{
    // 경기 도중 들어온 경우(재접속 등)에는 알림음 없이 배경음만 맞춘다.
    const bool bPlayStinger = OldPhase != EGamePhase::None;

    switch (NewPhase)
    {
    case EGamePhase::HideTime:
        PlayMusic(HideTimeMusic);
        break;

    case EGamePhase::Phase1:
    case EGamePhase::Phase2:
        if (bPlayStinger)
        {
            PlayUISound(PhaseStartSound);
        }
        StopFeverLoop();
        PlayMusic(PhaseMusic);
        break;

    case EGamePhase::ForcedSwap:
        if (bPlayStinger)
        {
            PlayUISound(ForcedSwapSound);
        }
        PlayMusic(PhaseMusic);
        break;

    case EGamePhase::FeverTime:
        if (bPlayStinger)
        {
            PlayUISound(PhaseStartSound);
        }
        PlayMusic(FeverMusic);
        if (FeverLoopSound && !FeverLoopComponent)
        {
            FeverLoopComponent = UGameplayStatics::SpawnSound2D(this, FeverLoopSound, FeverLoopVolume, 1.f, 0.f, nullptr, false, true);
        }
        break;

    case EGamePhase::Ended:
        // 승리/패배 소리는 결과 화면(ShowGameResultLocal)에서 재생
        StopFeverLoop();
        StopMusic();
        break;

    default:
        break;
    }
}

void AMyPlayerController::PlayMusic(USoundBase* Music)
{
    if (!IsLocalController())
    {
        return;
    }

    // 같은 곡이 이미 재생 중이면 끊지 않고 이어서 재생
    if (Music && Music == CurrentMusic && IsValid(MusicComponent) && MusicComponent->IsPlaying())
    {
        return;
    }

    StopMusic();

    if (!Music)
    {
        return;
    }

    // 레벨 이동 시 자동 정리되도록 bPersistAcrossLevelTransition=false
    const float TrackVolume = MusicVolume * (Music == WaitRoomMusic ? WaitRoomMusicVolumeScale : 1.f);
    MusicComponent = UGameplayStatics::SpawnSound2D(this, Music, TrackVolume, 1.f, 0.f, nullptr, false, true);
    if (MusicComponent && MusicFadeSeconds > 0.f)
    {
        MusicComponent->FadeIn(MusicFadeSeconds, TrackVolume);
    }
    CurrentMusic = Music;
}

void AMyPlayerController::StopMusic()
{
    if (IsValid(MusicComponent))
    {
        if (MusicFadeSeconds > 0.f)
        {
            MusicComponent->FadeOut(MusicFadeSeconds, 0.f);
        }
        else
        {
            MusicComponent->Stop();
        }
    }

    MusicComponent = nullptr;
    CurrentMusic = nullptr;
}

void AMyPlayerController::StopFeverLoop()
{
    if (IsValid(FeverLoopComponent))
    {
        FeverLoopComponent->FadeOut(0.5f, 0.f);
    }
    FeverLoopComponent = nullptr;
}

void AMyPlayerController::PlayUISound(USoundBase* Sound, float Volume)
{
    if (!Sound || !IsLocalController())
    {
        return;
    }

    UGameplayStatics::PlaySound2D(this, Sound, Volume);
}

// ===== 유령 관전 =====

void AMyPlayerController::ToggleSpectate()
{
    if (bSpectating)
    {
        StopSpectate();
    }
    else
    {
        StartSpectate();
    }
}

void AMyPlayerController::SpectateNext()
{
    if (bSpectating)
    {
        CycleSpectateTarget(+1);
    }
}

void AMyPlayerController::SpectatePrev()
{
    if (bSpectating)
    {
        CycleSpectateTarget(-1);
    }
}

void AMyPlayerController::GatherSpectateTargets(TArray<AMyCharacter*>& OutTargets) const
{
    OutTargets.Reset();
    if (!GetWorld())
    {
        return;
    }

    for (TActorIterator<AMyCharacter> It(GetWorld()); It; ++It)
    {
        AMyCharacter* MC = *It;
        if (IsValid(MC) && !MC->IsEliminated() && MC->GetPlayerState())
        {
            OutTargets.Add(MC);
        }
    }

    // 술래 먼저, 그다음 이름 순(누를 때마다 순서가 바뀌지 않게)
    OutTargets.Sort([](const AMyCharacter& A, const AMyCharacter& B)
    {
        if (A.IsSeeker() != B.IsSeeker())
        {
            return A.IsSeeker();
        }
        const AMyPlayerState* PA = A.GetPlayerState<AMyPlayerState>();
        const AMyPlayerState* PB = B.GetPlayerState<AMyPlayerState>();
        return (PA ? PA->GetNickname() : FString()) < (PB ? PB->GetNickname() : FString());
    });
}

FVector AMyPlayerController::GetSpectatePivot(const AMyCharacter* Target) const
{
    if (!Target)
    {
        return FVector::ZeroVector;
    }

    // 물체로 숨어 있으면 그 물체 가운데를 본다
    if (const APropBase* Prop = Target->GetPossessedProp())
    {
        FVector Origin;
        FVector Extent;
        Prop->GetActorBounds(true, Origin, Extent);
        return Origin;
    }

    return Target->GetActorLocation() + FVector(0.f, 0.f, 60.f);
}

void AMyPlayerController::StartSpectate()
{
    if (!IsLocalController() || bSpectating || SettingsWidget)
    {
        return;
    }

    AGhostCharacter* Ghost = GetGhostCharacter();
    if (!Ghost)
    {
        return;
    }

    TArray<AMyCharacter*> Targets;
    GatherSpectateTargets(Targets);
    if (Targets.Num() == 0)
    {
        return;
    }

    // 놓기 중이었으면 취소(관전 중에는 물체를 놓지 않는다)
    if (Ghost->IsPlacementMode())
    {
        Ghost->CancelPlacementMode();
    }

    bSpectating = true;
    SpectateTarget = Targets[0];

    // 유령은 관전하는 동안 움직이지 않는다
    SetIgnoreMoveInput(true);

    SpectateYaw = GetControlRotation().Yaw;
    SpectatePitch = -15.f;
    SpectatePivot = GetSpectatePivot(SpectateTarget.Get());

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.ObjectFlags |= RF_Transient;
    SpectateCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, Params);
    if (SpectateCamera)
    {
        SpectateCamera->SetReplicates(false);
        if (UCameraComponent* Cam = SpectateCamera->GetCameraComponent())
        {
            Cam->bConstrainAspectRatio = false;
            Cam->SetFieldOfView(90.f);
        }
        UpdateSpectate(0.f);
        SetViewTargetWithBlend(SpectateCamera, 0.3f);
    }

    // 마우스로 둘러보도록 커서를 숨기고 게임 입력만
    if (GhostUIWidgetInstance)
    {
        GhostUIWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
    }
    bShowMouseCursor = false;
    bEnableClickEvents = false;
    bEnableMouseOverEvents = false;
    SetInputMode(FInputModeGameOnly());

    if (!SpectateHud)
    {
        SpectateHud = CreateWidget<USpectateHudWidget>(this, USpectateHudWidget::StaticClass());
    }
    if (SpectateHud && !SpectateHud->IsInViewport())
    {
        SpectateHud->AddToViewport(820);
    }
    RefreshSpectateHud();
}

void AMyPlayerController::StopSpectate()
{
    if (!bSpectating)
    {
        return;
    }

    bSpectating = false;
    SpectateTarget = nullptr;
    SetIgnoreMoveInput(false);

    if (APawn* MyPawn = GetPawn())
    {
        SetViewTargetWithBlend(MyPawn, 0.25f);
    }

    if (SpectateCamera)
    {
        SpectateCamera->Destroy();
        SpectateCamera = nullptr;
    }

    if (SpectateHud)
    {
        SpectateHud->RemoveFromParent();
    }

    // 유령 화면(커서 + 물체 아이콘)으로 돌아간다
    if (GhostUIWidgetInstance)
    {
        GhostUIWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
    if (GetGhostCharacter())
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(Mode);
        bShowMouseCursor = true;
        bEnableClickEvents = true;
        bEnableMouseOverEvents = true;
    }
}

void AMyPlayerController::CycleSpectateTarget(int32 Direction)
{
    TArray<AMyCharacter*> Targets;
    GatherSpectateTargets(Targets);
    if (Targets.Num() == 0)
    {
        StopSpectate();
        return;
    }

    int32 Index = Targets.IndexOfByKey(SpectateTarget.Get());
    Index = Index == INDEX_NONE ? 0 : (Index + Direction + Targets.Num()) % Targets.Num();
    SpectateTarget = Targets[Index];
    RefreshSpectateHud();
}

void AMyPlayerController::RefreshSpectateHud()
{
    AMyCharacter* Target = SpectateTarget.Get();
    if (!SpectateHud || !Target)
    {
        return;
    }

    TArray<AMyCharacter*> Targets;
    GatherSpectateTargets(Targets);

    const AMyPlayerState* PS = Target->GetPlayerState<AMyPlayerState>();
    FString Name = PS ? PS->GetNickname() : FString();
    if (Name.IsEmpty() && PS)
    {
        Name = PS->GetPlayerName();
    }

    const bool bSeeker = Target->IsSeeker();
    const FString RoleLabel = bSeeker
        ? FString(TEXT("술래"))
        : (Target->IsPossessing() ? FString(TEXT("숨는 사람 · 숨는 중")) : FString(TEXT("숨는 사람")));
    const FLinearColor RoleColor = bSeeker
        ? FLinearColor::FromSRGBColor(FColor(255, 120, 120))
        : FLinearColor::FromSRGBColor(FColor(120, 220, 150));

    SpectateHud->SetTarget(Name, RoleLabel, RoleColor, Targets.IndexOfByKey(Target), Targets.Num());
}

void AMyPlayerController::UpdateSpectate(float DeltaTime)
{
    // 유령이 아니게 되면(게임 끝, 맵 이동 등) 관전을 끈다
    if (!GetGhostCharacter())
    {
        StopSpectate();
        return;
    }

    // 보던 사람이 잡히거나 나가면 다음 사람으로
    AMyCharacter* Target = SpectateTarget.Get();
    if (!IsValid(Target) || Target->IsEliminated())
    {
        CycleSpectateTarget(+1);
        Target = SpectateTarget.Get();
        if (!bSpectating || !Target)
        {
            return;
        }
    }

    // 마우스로 대상 주위를 돈다(설정 창이 열려 있으면 멈춤)
    if (!SettingsWidget)
    {
        float DX = 0.f;
        float DY = 0.f;
        GetInputMouseDelta(DX, DY);
        const float Sens = 2.0f * MouseSensitivity;
        SpectateYaw += DX * Sens;
        SpectatePitch = FMath::Clamp(SpectatePitch + DY * Sens * (bInvertMouseY ? -1.f : 1.f), -70.f, 30.f);
    }

    // 숨는 중 표시가 바뀔 수 있어 0.5초마다 다시 쓴다
    SpectateHudRefreshElapsed += DeltaTime;
    if (SpectateHudRefreshElapsed >= 0.5f)
    {
        SpectateHudRefreshElapsed = 0.f;
        RefreshSpectateHud();
    }

    const FVector WantedPivot = GetSpectatePivot(Target);
    SpectatePivot = DeltaTime > 0.f
        ? FMath::VInterpTo(SpectatePivot, WantedPivot, DeltaTime, 12.f)
        : WantedPivot;

    constexpr float Distance = 420.f;
    const FRotator ViewRot(SpectatePitch, SpectateYaw, 0.f);
    FVector CamLoc = SpectatePivot - ViewRot.Vector() * Distance;

    // 벽에 막히면 카메라를 당긴다
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SpectateCamera), false);
    Params.AddIgnoredActor(Target);
    if (AActor* Prop = Target->GetPossessedProp())
    {
        Params.AddIgnoredActor(Prop);
    }
    Params.AddIgnoredActor(GetPawn());
    FHitResult Hit;
    if (GetWorld()->SweepSingleByChannel(Hit, SpectatePivot, CamLoc, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12.f), Params))
    {
        CamLoc = Hit.Location;
    }

    if (SpectateCamera)
    {
        SpectateCamera->SetActorLocationAndRotation(CamLoc, ViewRot);
    }
}
