#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PasswordDoor.generated.h"

class UBillboardComponent;

UCLASS()
class HIDE_API APasswordDoor : public AActor
{
    GENERATED_BODY()

public:
    APasswordDoor();

    UFUNCTION(BlueprintCallable)
    bool CanInteractFrom(const AActor* Interactor) const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Password")
    float InteractionDistance = 250.f;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBillboardComponent* BillboardComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billboard Settings")
    UTexture2D* CustomSpriteTexture;

};
