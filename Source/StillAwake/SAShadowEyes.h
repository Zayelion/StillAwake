#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SAShadowEyes.generated.h"

class ASAHouse;
class UPrimitiveComponent;
class UStaticMeshComponent;

UCLASS()
class STILLAWAKE_API ASAShadowEyes : public AActor
{
    GENERATED_BODY()

  public:
    ASAShadowEyes();
    void Observe(const ASAHouse *House, const FVector &Origin, const FVector &Direction, const AActor *Viewer,
                 float Delta);
    void Reset();

  private:
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> EyeParts;
    TWeakObjectPtr<UPrimitiveComponent> SurfaceComponent;
    FVector FocusPoint = FVector::ZeroVector;
    FVector FocusNormal = FVector::UpVector;
    double LookSeconds = 0;
    float TimeUntilLightCheck = 0;
    bool bShadowed = false;

    bool IsShadowed(const ASAHouse &House, const FHitResult &Hit, const AActor *Viewer) const;
    void PrepareEyes(const ASAHouse &House);
    void AnimateEyes();
};
