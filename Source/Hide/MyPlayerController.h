#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "MyPlayerState.h"
#include "MyGameState.h"
#include "MyPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class AMyCharacter;
class APropBase;
class AGhostCharacter;
class AMyPlayerState;
class AMyGameState;
class APasswordDoor;
class UDestroyProgressWidget;
class USoundBase;
class UAudioComponent;
class USoundClass;
class USoundMix;
class USettingsWidget;
class UHitFlashWidget;
class UEliminationNoticeWidget;
class UControlsGuideWidget;
class UReturnCountdownWidget;
class UCrosshairWidget;
class UScoreboardWidget;
class UResultScoreWidget;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

enum class EVolumeChannel : uint8
{
    Master,
    Music,
    SFX
};

UCLASS()
class HIDE_API AMyPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMyPlayerController();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;

    // ===== 볼륨 설정 (로컬 전용, GameUserSettings.ini에 저장) =====
    float GetVolumeSetting(EVolumeChannel Channel) const;
    void SetVolumeSetting(EVolumeChannel Channel, float Value);
    void ToggleSettingsMenu();
    void CloseSettingsMenu();

    // 누르고 있던 공격 입력(길게 누르기 포함)을 취소한다. 설정 창을 열고 닫을 때 사용.
    void CancelAttackInput();

    UFUNCTION(BlueprintCallable)
    void UI_RequestStartGame();

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_Attack = nullptr;

    UFUNCTION(BlueprintCallable)
    void UI_SelectGhostRespawnIndex(int32 InIndex);

    UFUNCTION(BlueprintCallable)
    void UpdateGameRoomHearts(int32 InHitCount);

    UFUNCTION(BlueprintCallable)
    void UpdateGameRoomClues(const TArray<int32>& InPositionedClueNumbers);

    UFUNCTION(BlueprintCallable)
    void UpdateGhostUI();

    UFUNCTION(BlueprintCallable)
    void UI_SubmitPasswordCode(const TArray<int32>& InputCode);

    UFUNCTION(BlueprintCallable)
    void UI_ClosePasswordInput();

    UFUNCTION(BlueprintCallable)
    bool IsGhostUIOpen() const { return GhostUIWidgetInstance != nullptr; }
    
    UFUNCTION(BlueprintCallable)
    void HideGameRoomUI();

    void ShowDestroyProgressUI(AMyCharacter* SourceCharacter);
    void HideDestroyProgressUI();

    virtual void OnRep_Pawn() override;

    // 경기 중 접속이 끊기면 캐릭터를 파괴하지 않고 남겨 재접속 시 이어서 조종하게 한다.
    virtual void PawnLeavingGame() override;

    UFUNCTION(Client, Reliable)
    void ClientFinalizeGhostTransition();

    // 재접속 직후 게임 UI(하트/단서)를 현재 상태로 다시 그린다.
    UFUNCTION(Client, Reliable)
    void ClientRestoreAfterReconnect(int32 SeekerHitCount);

    UFUNCTION(Client, Reliable)
    void ClientSetForcedSwapWarning(bool bShowWarning, double RemainingSeconds);

    // 맞았을 때 화면 가장자리 빨간 효과(맞은 본인만). 탈락이면 더 진하고 길게.
    UFUNCTION(Client, Unreliable)
    void ClientPlayHitFlash(bool bEliminated);

    // 러너 탈락 알림(모든 플레이어 화면 위쪽)
    UFUNCTION(Client, Reliable)
    void ClientShowEliminationNotice(const FString& RunnerName, int32 RemainingRunners);

    UFUNCTION(Client, Reliable)
    void ClientShowGameResult(EFinalRole WinningRole);

    // 같은 팀 점수(서버가 0.5초마다 보냄). 상대 팀 점수는 게임이 끝날 때까지 받지 않는다.
    UFUNCTION(Client, Unreliable)
    void ClientReceiveTeamScores(const TArray<FTeamScoreEntry>& Entries);

    // 점수판에 보여도 되는 점수. 모르는(가려진) 점수면 false.
    bool GetVisibleScoreStats(const AMyPlayerState* PS, FHideScoreStats& OutStats) const;

    void ShowGameResultLocal(EFinalRole WinningRole);

    // 결과 화면의 "바로 대기실로" 버튼(방장 전용). 서버에서 방장인지 다시 확인한다.
    void RequestReturnToWaitRoomNow();

    UFUNCTION(Server, Reliable)
    void ServerRequestReturnToWaitRoom();

    UFUNCTION(Client, Reliable)
    void ClientEnterWaitRoomUI();

    UFUNCTION(BlueprintImplementableEvent, Category = "Game Result")
    void OnGameResultReceived(EFinalRole WinningRole, bool bWon);
protected:
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputMappingContext* IMC_Default = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_Possess = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_Lock = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> WBP_WaitRoom = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_GhostPlacementRefresh = nullptr;

    APropBase* GetAimPropUnderCrosshair() const;
    bool GetScreenRayHit(FHitResult& OutHit, bool bPreferCursor) const;

private:
    void OnPossessPressed(const FInputActionValue& Value);
    void OnLockPressed(const FInputActionValue& Value);
    void OnGhostPlacementRefreshPressed(const FInputActionValue& Value);

    void UpdateHovered();
    void ApplyOutline(AActor* NewHovered);
    void ClearOutline(AActor* ActorToClear);

    AMyCharacter* GetMyCharacter() const;
    APropBase* GetHoveredProp() const;
    APasswordDoor* GetHoveredPasswordDoor() const;
    bool TryOpenPasswordInput();
    void UpdatePasswordInputState();
    void BroadcastPasswordCooldownUpdated(double CooldownRemaining);

    UFUNCTION(Server, Reliable)
    void ServerSubmitPasswordCode(const TArray<int32>& InputCode);

    bool CanSubmitPasswordCode_Server(const AMyGameState* GS) const;

    // 서버측 문 거리 검사에 더하는 여유 거리(네트워크 지연 보정).
    UPROPERTY(EditDefaultsOnly, Category = "Password")
    float PasswordSubmitDistanceTolerance = 100.f;

    UFUNCTION(Client, Reliable)
    void ClientReceivePasswordAttemptResult(int32 CorrectCount, bool bSuccess, double CooldownRemaining);

    UPROPERTY()
    AActor* HoveredActor = nullptr;

    UPROPERTY()
    AActor* OutlinedActor = nullptr;

    UPROPERTY()
    UUserWidget* WaitRoomWidgetInstance = nullptr;

    void ShowWaitRoomUI();

    UFUNCTION(Server, Reliable)
    void ServerRequestStartGame();

    UFUNCTION(Client, Reliable)
    void ClientCloseWaitRoomUI();

    void CloseWaitRoomUI();
    void OnAttackPressed(const FInputActionValue& Value);
    void OnAttackReleased(const FInputActionValue& Value);
    void HandleAttackHoldTriggered();

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    float AttackHoldThreshold = 0.25f; // 이 시간 이상 누르면 "오브젝트 파괴"로 판정

    FTimerHandle TH_AttackHold;
    bool bHoldTriggered = false;

    UPROPERTY()
    APropBase* AttackHoldTarget = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> WBP_GameRoom = nullptr;

    UPROPERTY()
    UUserWidget* GameRoomWidgetInstance = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UDestroyProgressWidget> WBP_DestroyProgress = nullptr;

    UPROPERTY()
    UDestroyProgressWidget* DestroyProgressWidgetInstance = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> WBP_PasswordInput = nullptr;

    UPROPERTY()
    UUserWidget* PasswordInputWidgetInstance = nullptr;

    UPROPERTY()
    APasswordDoor* ActivePasswordDoor = nullptr;

    void ShowGameRoomUI();
    void CloseGameRoomUI();
    void UpdateGameRoomTeamStatus();
    void UpdateGameTimeWidgets();
    void ShowPasswordInputUI();
    void ClosePasswordInputUI();
    void RefreshGameRoomCluesFromPlayerState();

    float TeamStatusRefreshElapsed = 0.f;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    float TeamStatusRefreshInterval = 0.25f;

    AGhostCharacter* GetGhostCharacter() const;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> WBP_GhostUI = nullptr;

    UPROPERTY()
    UUserWidget* GhostUIWidgetInstance = nullptr;
    void ShowGhostUI();

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> WBP_GameResult = nullptr;

    UPROPERTY()
    UUserWidget* GameResultWidgetInstance = nullptr;

    TWeakObjectPtr<AMyGameState> LastGameResultSource;

    // 결과 화면 위에 띄우는 대기실 복귀 카운트다운 + 방장 버튼
    UPROPERTY(Transient)
    UReturnCountdownWidget* ReturnCountdownWidget = nullptr;

    // 결과 화면 오른쪽 "이번 판 결과" 카드(MVP, 팀별 순위, 칭호)
    UPROPERTY(Transient)
    UResultScoreWidget* ResultScoreWidget = nullptr;

    // 화면 중앙 조준점(게임 화면 UI와 같이 띄우고 닫는다)
    UPROPERTY(Transient)
    UCrosshairWidget* CrosshairWidget = nullptr;

    // ===== 페이즈 소리 / 배경음 (각 클라 로컬에서만 재생) =====
    UPROPERTY(EditDefaultsOnly, Category = "Sound|Music")
    USoundBase* WaitRoomMusic = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound|Music")
    USoundBase* HideTimeMusic = nullptr;

    // 1페이즈~2페이즈(강제 교체 포함) 동안 이어서 재생
    UPROPERTY(EditDefaultsOnly, Category = "Sound|Music")
    USoundBase* PhaseMusic = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound|Music")
    USoundBase* FeverMusic = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound|Music", meta = (ClampMin = "0.0"))
    float MusicVolume = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Sound|Music", meta = (ClampMin = "0.0"))
    float MusicFadeSeconds = 1.0f;

    // 술래가 풀려날 때(1페이즈), 2페이즈, 피버 시작 시
    UPROPERTY(EditDefaultsOnly, Category = "Sound|Phase")
    USoundBase* PhaseStartSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound|Phase")
    USoundBase* ForcedSwapSound = nullptr;

    // 피버 동안 배경음 위에 겹쳐 반복 재생(선택)
    UPROPERTY(EditDefaultsOnly, Category = "Sound|Phase")
    USoundBase* FeverLoopSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound|Phase", meta = (ClampMin = "0.0"))
    float FeverLoopVolume = 0.4f;

    // 경기 종료 직전 마지막 N초 동안 1초마다
    UPROPERTY(EditDefaultsOnly, Category = "Sound|Phase")
    USoundBase* LastSecondsTickSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound|Phase", meta = (ClampMin = "0"))
    int32 LastSecondsTickCount = 10;

    // 비밀번호 제출 결과(제출한 본인만 들림)
    UPROPERTY(EditDefaultsOnly, Category = "Sound|Password")
    USoundBase* PasswordCorrectSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound|Password")
    USoundBase* PasswordWrongSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound|Result")
    USoundBase* WinSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Sound|Result")
    USoundBase* LoseSound = nullptr;

    UPROPERTY()
    UAudioComponent* MusicComponent = nullptr;

    UPROPERTY()
    UAudioComponent* FeverLoopComponent = nullptr;

    UPROPERTY()
    USoundBase* CurrentMusic = nullptr;

    TWeakObjectPtr<AMyGameState> LastSoundGameState;
    EGamePhase LastSoundPhase = EGamePhase::None;
    int32 LastTickSecond = -1;

    void UpdatePhaseSounds();
    void HandlePhaseChangedLocal(EGamePhase OldPhase, EGamePhase NewPhase);
    void PlayMusic(USoundBase* Music);
    void StopMusic();
    void StopFeverLoop();
    void PlayUISound(USoundBase* Sound, float Volume = 1.f);

    // ===== 볼륨 설정 =====
    // 사운드 웨이브에 지정된 Sound Class(배경음/효과음)별로 볼륨을 덮어쓴다
    UPROPERTY(EditDefaultsOnly, Category = "Sound|Volume")
    TSoftObjectPtr<USoundClass> MusicSoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/Game/Game/Sounds/Classes/SC_Music.SC_Music")));

    UPROPERTY(EditDefaultsOnly, Category = "Sound|Volume")
    TSoftObjectPtr<USoundClass> SFXSoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/Game/Game/Sounds/Classes/SC_SFX.SC_SFX")));

    UPROPERTY(Transient)
    USoundMix* VolumeMix = nullptr;

    UPROPERTY(Transient)
    USettingsWidget* SettingsWidget = nullptr;

    float VolumeMaster = 1.f;
    float VolumeMusic = 1.f;
    float VolumeSFX = 1.f;
    bool bSettingsPrevShowCursor = false;

    void LoadVolumeSettings();
    void SaveVolumeSettings() const;
    void ApplyVolumeSettings();

    UPROPERTY(Transient)
    UHitFlashWidget* HitFlashWidget = nullptr;

    UPROPERTY(Transient)
    UEliminationNoticeWidget* EliminationNoticeWidget = nullptr;

    // ===== 대기실 닉네임: 입력하는 대로 서버에 보낸다(확인 버튼 없음) =====
    UFUNCTION()
    void OnNicknameTextChanged(const FText& Text);

    UFUNCTION()
    void OnNicknameTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    void SendPendingNickname();

    FTimerHandle TH_NicknameSend;
    FString PendingNickname;
    FString LastSentNickname;

    // ===== 역할별 조작법 안내(게임 맵에서만, H로 열고 닫기) =====
    UPROPERTY(Transient)
    UControlsGuideWidget* ControlsGuideWidget = nullptr;

    void UpdateControlsGuide();
    void ToggleControlsGuide();

    // ===== 점수판(게임 맵에서 Tab을 누르고 있는 동안) =====
    UPROPERTY(Transient)
    UScoreboardWidget* ScoreboardWidget = nullptr;

    void ShowScoreboard();
    void HideScoreboard();

    // WBP_GameRoom 위쪽 프레임 아래 빈 홈에 게임 전체 타임라인 바를 끼워 넣는다
    void AttachTimelineBar();

    TMap<TWeakObjectPtr<const APlayerState>, FHideScoreStats> TeamScoreCache;

    // ===== 치트(테스트용, Shipping 빌드에서는 동작 안 함) =====
    // Ctrl+Shift+C: 단서가 배치된 프롭 테두리 강조 + 비밀번호 정답 표시 (다시 누르면 끔)
    void ToggleClueCheat();

    UFUNCTION(Server, Reliable)
    void ServerRequestClueCheat();

    UFUNCTION(Client, Reliable)
    void ClientReceiveClueCheat(const TArray<APropBase*>& ClueProps, const TArray<int32>& Code);

    void UpdateClueCheat();
    void ClearClueCheatOutlines();

    void DestroyClueCheatBeams();

    // 치트 강조에 쓸 커스텀 뎁스 스텐실 값. M_OutlineRGB_PP: 1 초록, 2 파랑, 3 빨강, 5 금색(치트 전용)
    // 4는 BP_MyCharacter 메시가 쓰고 있어서 피한다.
    UPROPERTY(EditDefaultsOnly, Category = "Cheat")
    int32 ClueCheatStencil = 5;

    // 신호기 빛기둥: 단서 프롭 위치에서 하늘로 뻗는 기둥. 치트를 쓴 플레이어 화면에만 만든다(복제 안 함).
    UPROPERTY(EditDefaultsOnly, Category = "Cheat")
    TObjectPtr<UStaticMesh> ClueBeaconMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Cheat")
    TObjectPtr<UMaterialInterface> ClueBeaconMaterial = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Cheat")
    float ClueBeaconHeight = 20000.f;

    UPROPERTY(EditDefaultsOnly, Category = "Cheat")
    float ClueBeaconWidth = 60.f;

    bool bClueCheatOn = false;
    TArray<TWeakObjectPtr<APropBase>> ClueCheatProps;
    TArray<int32> ClueCheatCode;

    // ClueCheatProps와 같은 순서
    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> ClueCheatBeams;
};


