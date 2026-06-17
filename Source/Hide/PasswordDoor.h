#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PasswordDoor.generated.h"

class UBillboardComponent;
class UBoxComponent;

UCLASS()
class HIDE_API APasswordDoor : public AActor
{
    GENERATED_BODY()

public:
    APasswordDoor();

    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UFUNCTION(BlueprintCallable)
    bool CanInteractFrom(const AActor* Interactor) const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Password")
    float InteractionDistance = 250.f;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* InteractionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBillboardComponent* BillboardComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billboard Settings")
    FVector InteractionBoxExtent = FVector(80.f, 20.f, 50.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billboard Settings")
    UTexture2D* CustomSpriteTexture;

private:
    void ApplyBillboardSettings();
};
