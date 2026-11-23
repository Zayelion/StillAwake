#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Core/SASprintHorror.h"
#include "SACharacter.generated.h"
class UCameraComponent;
UCLASS()
class STILLAWAKE_API ASACharacter : public ACharacter
{
    GENERATED_BODY()
  public:
    ASACharacter();
    virtual void SetupPlayerInputComponent(UInputComponent *Input) override;
    virtual void Tick(float Delta) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    static float MovementSpeed(float Walk, bool Running)
    {
        return Walk * (Running ? 2.f : 1.f);
    }
    void Place(const FVector &Position, float Yaw);
    void SetBedView(const FVector &Position, const FRotator &Rotation);
    SA::GlimpseFrame Glimpse(bool ReducedIntensity) const { return SprintEffect.Frame(ReducedIntensity); }

  private:
    FVector Previous = FVector::ZeroVector;
    bool bPositionReady = false;
    bool bInBed = false;
    FRotator BedRotation;
    float FootstepDistance = 0, BobPhase = 0;
    SA::SprintHorror SprintEffect;
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void Look(float Value);
    void AdjustBedLook(float Yaw, float Pitch);
};
UCLASS()
class STILLAWAKE_API ASAPlayerController : public APlayerController
{
    GENERATED_BODY()
  public:
    virtual void SetupInputComponent() override;

  private:
    void Escape();
    void Click();
    void Interact();
    void PreviousPage();
    void NextPage();
    void Confirm();
};
