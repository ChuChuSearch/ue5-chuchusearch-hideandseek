#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyPlayerState.h"
#include "LobbyDevices.generated.h"

class AMyPlayerController;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

// 대기실 로비 맵에 놓는 장치들의 공통 부모.
// - 가까이 가면 화면 아래에 "E  ○○" 안내가 뜨고, E를 누르면 상호작용한다.
// - 머리 위 표지판(제목 + 한 줄 설명)은 로비 HUD가 화면에 그린다.
// - 모양은 엔진 기본 도형(상자/원기둥)에 색만 입힌 것. 에디터에서 위치·크기만 맞추면 된다.
UCLASS(Abstract)
class HIDE_API ALobbyDevice : public AActor
{
    GENERATED_BODY()

public:
    ALobbyDevice();

    // 이 사람에게 보여 줄 상호작용 안내. 안내가 없으면 false.
    // bOutEnabled가 false면 흐리게(지금은 할 수 없는 상태) 보여 준다.
    virtual bool GetPrompt(const AMyPlayerController* PC, FString& OutText, bool& bOutEnabled) const { return false; }

    // 로컬에서 E를 눌렀을 때. true를 돌려주면 서버에도 알린다(InteractServer).
    virtual bool InteractLocal(AMyPlayerController* PC) { return true; }

    // 서버에서 실제 처리(거리 확인은 컨트롤러가 이미 했다)
    virtual void InteractServer(AMyPlayerController* PC) {}

    // 표지판
    virtual FString GetLabelTitle() const { return LabelTitle; }
    virtual FString GetLabelSub(const AMyPlayerController* PC) const { return FString(); }
    virtual FLinearColor GetLabelColor() const { return LabelColor; }
    FVector GetLabelLocation() const { return GetActorLocation() + FVector(0.f, 0.f, LabelHeight); }

    // 상호작용할 수 있는 거리 안인지(수평 거리 + 높이 차)
    bool IsPawnInRange(const APawn* Pawn) const;

    // 거리 비교용
    float GetHorizontalDistanceTo(const APawn* Pawn) const;

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

protected:
    // 역할 문처럼 설정값에 따라 이름/색이 바뀌는 장치가 여기서 맞춘다(에디터 배치 때와 게임 시작 때 둘 다)
    virtual void RefreshAppearance() {}

    UPROPERTY(VisibleAnywhere, Category = "Lobby")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(EditAnywhere, Category = "Lobby")
    FString LabelTitle;

    UPROPERTY(EditAnywhere, Category = "Lobby")
    FLinearColor LabelColor = FLinearColor(0.5f, 0.3f, 0.9f);

    UPROPERTY(EditAnywhere, Category = "Lobby", meta = (ClampMin = "50.0"))
    float InteractRadius = 260.f;

    UPROPERTY(EditAnywhere, Category = "Lobby")
    float LabelHeight = 320.f;

    // 도형 하나 만들기(생성자에서). Color는 OnConstruction에서 입힌다.
    UStaticMeshComponent* AddShape(FName Name, bool bCylinder, const FVector& Location, const FVector& SizeCm, bool bCollide);

    // 도형별 색(AddShape 순서대로). 자식 클래스가 OnConstruction 전에 바꿀 수 있다.
    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> Shapes;

    UPROPERTY(EditAnywhere, Category = "Lobby|Look")
    TArray<FLinearColor> ShapeColors;

    void ApplyShapeColors();

    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CylinderMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> ShapeMaterial;
};

// 출발 발판: 위에 서 있으면 준비 완료(내려오면 준비 취소).
// 방장이 발판 위에서 E를 누르면 짧게 카운트다운하고 게임을 시작한다.
UCLASS()
class HIDE_API ALobbyStartPad : public ALobbyDevice
{
    GENERATED_BODY()

public:
    ALobbyStartPad();

    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual bool GetPrompt(const AMyPlayerController* PC, FString& OutText, bool& bOutEnabled) const override;
    virtual void InteractServer(AMyPlayerController* PC) override;
    virtual FString GetLabelSub(const AMyPlayerController* PC) const override;

    bool IsPawnOnPad(const APawn* Pawn) const;
    bool IsCountingDown() const { return CountdownEndTime > 0.f; }

    // 출발까지 남은 초(카운트다운 중이 아니면 -1)
    float GetCountdownRemaining() const;

protected:
    // 발판 반지름(cm). 이 안에 서 있으면 준비.
    UPROPERTY(EditAnywhere, Category = "Lobby", meta = (ClampMin = "50.0"))
    float PadRadius = 230.f;

    UPROPERTY(EditAnywhere, Category = "Lobby", meta = (ClampMin = "0.0"))
    float CountdownSeconds = 3.f;

    // 서버 시각 기준 출발 시각(0이면 카운트다운 아님)
    UPROPERTY(Replicated)
    float CountdownEndTime = 0.f;

    TWeakObjectPtr<AMyPlayerController> CountdownHost;

    void CancelCountdown(const FString& Reason);
    void BroadcastNotice(const FString& Message) const;

    // 손님들이 모두 발판 위인지(클라이언트도 계산할 수 있게 PlayerState의 준비 상태로)
    void CountPlayers(int32& OutPlayers, int32& OutGuests, int32& OutReadyGuests) const;
};

// 역할 문: 문 앞에서 E를 누르면 그 역할을 지망한다(다시 누르면 상관없음).
UCLASS()
class HIDE_API ALobbyRoleDoor : public ALobbyDevice
{
    GENERATED_BODY()

public:
    ALobbyRoleDoor();


    virtual bool GetPrompt(const AMyPlayerController* PC, FString& OutText, bool& bOutEnabled) const override;
    virtual bool InteractLocal(AMyPlayerController* PC) override;
    virtual FString GetLabelSub(const AMyPlayerController* PC) const override;

protected:
    UPROPERTY(EditAnywhere, Category = "Lobby")
    ERolePreference Preference = ERolePreference::SeekerWish;

    virtual void RefreshAppearance() override;
};

// 게임 설정 게시판: E로 게임 설정 창을 연다(방장만 바꿀 수 있고 나머지는 보기만).
UCLASS()
class HIDE_API ALobbySettingsBoard : public ALobbyDevice
{
    GENERATED_BODY()

public:
    ALobbySettingsBoard();

    virtual bool GetPrompt(const AMyPlayerController* PC, FString& OutText, bool& bOutEnabled) const override;
    virtual bool InteractLocal(AMyPlayerController* PC) override;
    virtual FString GetLabelSub(const AMyPlayerController* PC) const override;
};

// 유령 구멍: 사람이 E를 누르면 유령이 되어 날아다니며 물체 놓기를 연습하고,
// 유령으로 구멍 가까이 와서 다시 E를 누르면 사람으로 돌아온다.
UCLASS()
class HIDE_API ALobbyGhostHole : public ALobbyDevice
{
    GENERATED_BODY()

public:
    ALobbyGhostHole();

    virtual bool GetPrompt(const AMyPlayerController* PC, FString& OutText, bool& bOutEnabled) const override;
    virtual bool InteractLocal(AMyPlayerController* PC) override;
    virtual void InteractServer(AMyPlayerController* PC) override;
    virtual FString GetLabelSub(const AMyPlayerController* PC) const override;
};

// 구역 표지판: 상호작용 없이 머리 위 표지판만(예: "숨기 연습장"). 제목과 설명은 디테일 패널에서 바꾼다.
UCLASS()
class HIDE_API ALobbyZoneSign : public ALobbyDevice
{
    GENERATED_BODY()

public:
    ALobbyZoneSign();

    virtual FString GetLabelSub(const AMyPlayerController* PC) const override { return LabelSub; }

protected:
    UPROPERTY(EditAnywhere, Category = "Lobby")
    FString LabelSub;
};
