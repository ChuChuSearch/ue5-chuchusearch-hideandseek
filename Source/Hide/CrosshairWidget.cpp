#include "CrosshairWidget.h"

#include "MyCharacter.h"
#include "PropBase.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateBrush.h"

namespace
{
    // 크기(픽셀)
    constexpr float DotSize = 4.f;
    constexpr float LineLength = 9.f;
    constexpr float LineThickness = 2.f;
    constexpr float BaseGap = 6.f;     // 가운데에서 선이 시작하는 거리
    constexpr float KickGap = 7.f;     // 발사 직후 더 벌어지는 거리
    constexpr float OutlineSize = 1.f; // 어두운 테두리 두께

    // 술래 투사체와 같은 기준으로 대상을 찾는다(투사체는 WorldDynamic이라 이 채널을 막는 것에 부딪힌다)
    constexpr float TargetTraceDistance = 10000.f;

    const FLinearColor NormalColor(1.f, 1.f, 1.f, 1.f);
    const FLinearColor TargetColor = FLinearColor::FromSRGBColor(FColor(255, 96, 80));
    const FLinearColor OutlineColor(0.f, 0.f, 0.f, 0.65f);
}

UCrosshairWidget::UCrosshairWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DotBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
    DotBrush.TintColor = FSlateColor(FLinearColor::White);
    DotBrush.OutlineSettings = FSlateBrushOutlineSettings(DotSize * 0.5f);

    DotOutlineBrush = DotBrush;
    DotOutlineBrush.OutlineSettings = FSlateBrushOutlineSettings((DotSize + OutlineSize * 2.f) * 0.5f);
}

void UCrosshairWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCrosshairWidget::NotifyFired()
{
    Kick = 1.f;
}

void UCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateState(FMath::Min(InDeltaTime, 0.1f));
}

void UCrosshairWidget::UpdateState(float DeltaTime)
{
    APlayerController* PC = GetOwningPlayer();
    AMyCharacter* MC = PC ? Cast<AMyCharacter>(PC->GetPawn()) : nullptr;

    // 살아 있는 캐릭터로 조종 중이고, 마우스 커서를 쓰는 창(설정, 비밀번호 등)이 열려 있지 않을 때만 보인다
    bShow = MC && !MC->IsEliminated() && !PC->bShowMouseCursor;

    bOnTarget = false;
    bCoolingDown = false;

    if (bShow)
    {
        bCoolingDown = MC->IsSeeker() && MC->IsSeekerAttackOnCooldown();

        int32 ViewX = 0;
        int32 ViewY = 0;
        PC->GetViewportSize(ViewX, ViewY);

        FVector Origin;
        FVector Direction;
        if (ViewX > 0 && ViewY > 0 && PC->DeprojectScreenPositionToWorld(ViewX * 0.5f, ViewY * 0.5f, Origin, Direction))
        {
            FCollisionQueryParams Params(SCENE_QUERY_STAT(CrosshairTargetTrace), true);
            Params.AddIgnoredActor(MC);

            // 숨어 있는 물체(붙어 있는 액터)는 대상이 아니다
            TArray<AActor*> Attached;
            MC->GetAttachedActors(Attached);
            Params.AddIgnoredActors(Attached);
            if (AActor* Parent = MC->GetAttachParentActor())
            {
                Params.AddIgnoredActor(Parent);
            }

            FHitResult Hit;
            if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, Origin + Direction * TargetTraceDistance, ECC_WorldDynamic, Params))
            {
                AActor* HitActor = Hit.GetActor();
                bOnTarget = Cast<APropBase>(HitActor) != nullptr || Cast<AMyCharacter>(HitActor) != nullptr;
            }
        }
    }

    Kick = FMath::FInterpTo(Kick, 0.f, DeltaTime, 12.f);
    TargetBlend = FMath::FInterpTo(TargetBlend, bOnTarget ? 1.f : 0.f, DeltaTime, 18.f);
}

int32 UCrosshairWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

    if (!bShow)
    {
        return LayerId;
    }

    const FVector2f Center = AllottedGeometry.GetLocalSize() * 0.5f;
    const float Opacity = bCoolingDown ? 0.45f : 1.f;

    FLinearColor Color = FMath::Lerp(NormalColor, TargetColor, TargetBlend);
    Color.A *= Opacity;
    FLinearColor Outline = OutlineColor;
    Outline.A *= Opacity;

    const int32 OutlineLayer = LayerId + 1;
    const int32 FillLayer = LayerId + 2;

    // 가운데 기준 사각형 하나 그리기(테두리 먼저, 그 위에 채움)
    auto DrawRect = [&](const FVector2f& RectCenter, const FVector2f& Size)
    {
        const FVector2f OutlineSize2(Size.X + OutlineSize * 2.f, Size.Y + OutlineSize * 2.f);
        FSlateDrawElement::MakeBox(
            OutDrawElements, OutlineLayer,
            AllottedGeometry.ToPaintGeometry(OutlineSize2, FSlateLayoutTransform(RectCenter - OutlineSize2 * 0.5f)),
            &SolidBrush, ESlateDrawEffect::None, Outline);
        FSlateDrawElement::MakeBox(
            OutDrawElements, FillLayer,
            AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(RectCenter - Size * 0.5f)),
            &SolidBrush, ESlateDrawEffect::None, Color);
    };

    // 네 방향 선
    const float Gap = BaseGap + KickGap * Kick;
    const float Mid = Gap + LineLength * 0.5f;
    DrawRect(Center + FVector2f(0.f, -Mid), FVector2f(LineThickness, LineLength));
    DrawRect(Center + FVector2f(0.f, Mid), FVector2f(LineThickness, LineLength));
    DrawRect(Center + FVector2f(-Mid, 0.f), FVector2f(LineLength, LineThickness));
    DrawRect(Center + FVector2f(Mid, 0.f), FVector2f(LineLength, LineThickness));

    // 가운데 둥근 점
    const FVector2f DotOutline(DotSize + OutlineSize * 2.f, DotSize + OutlineSize * 2.f);
    const FVector2f Dot(DotSize, DotSize);
    FSlateDrawElement::MakeBox(
        OutDrawElements, OutlineLayer,
        AllottedGeometry.ToPaintGeometry(DotOutline, FSlateLayoutTransform(Center - DotOutline * 0.5f)),
        &DotOutlineBrush, ESlateDrawEffect::None, Outline);
    FSlateDrawElement::MakeBox(
        OutDrawElements, FillLayer,
        AllottedGeometry.ToPaintGeometry(Dot, FSlateLayoutTransform(Center - Dot * 0.5f)),
        &DotBrush, ESlateDrawEffect::None, Color);

    return FillLayer;
}
