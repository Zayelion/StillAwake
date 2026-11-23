#include "SACharacter.h"
#include "SAGameMode.h"
#include "SAHUD.h"
#include "SAAudio.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
ASACharacter::ASACharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(28, 88);
    GetCharacterMovement()->MaxWalkSpeed = 300;
    GetCharacterMovement()->JumpZVelocity = 0;
    GetCharacterMovement()->MaxStepHeight = 20;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    bUseControllerRotationYaw = true;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Eyes"));
    Camera->SetupAttachment(GetCapsuleComponent());
    Camera->SetRelativeLocation({0, 0, 77});
    Camera->bUsePawnControlRotation = true;
    Camera->FieldOfView = 80;
}
void ASACharacter::SetupPlayerInputComponent(UInputComponent *Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"), this, &ASACharacter::MoveForward);
    Input->BindAxis(TEXT("MoveRight"), this, &ASACharacter::MoveRight);
    Input->BindAxis(TEXT("Turn"), this, &ASACharacter::Turn);
    Input->BindAxis(TEXT("LookUp"), this, &ASACharacter::Look);
}
void ASACharacter::MoveForward(float V)
{
    auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>();
    if (G && G->CanMove())
        AddMovementInput(GetActorForwardVector(), V);
}
void ASACharacter::MoveRight(float V)
{
    auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>();
    if (G && G->CanMove())
        AddMovementInput(GetActorRightVector(), V);
}
void ASACharacter::Turn(float V)
{
    auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>();
    if (!G || !G->CanLook() || !G->Save || V == 0)
        return;
    const float Input = V * G->Save->Settings.Sensitivity;
    if (bInBed)
    {
        AdjustBedLook(Input, 0);
        return;
    }
    AddControllerYawInput(Input);
}
void ASACharacter::Look(float V)
{
    auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>();
    if (!G || !G->CanLook() || !G->Save || V == 0)
        return;
    const float Input = V * G->Save->Settings.Sensitivity * (G->Save->Settings.InvertY ? -1 : 1);
    if (bInBed)
    {
        AdjustBedLook(0, Input);
        return;
    }
    AddControllerPitchInput(Input);
}
void ASACharacter::AdjustBedLook(float Yaw, float Pitch)
{
    if (!Controller)
        return;
    const FRotator Offset = (Controller->GetControlRotation() - BedRotation).GetNormalized();
    Controller->SetControlRotation(BedRotation + FRotator(FMath::Clamp(Offset.Pitch + Pitch, -32.0, 27.0),
                                                          FMath::Clamp(Offset.Yaw + Yaw, -68.0, 68.0), 0));
}
void ASACharacter::Place(const FVector &P, float Yaw)
{
    GetCharacterMovement()->StopMovementImmediately();
    if (bInBed)
    {
        SetActorEnableCollision(true);
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        bInBed = false;
    }
    SetActorLocationAndRotation(P, FRotator(0, Yaw, 0), false, nullptr, ETeleportType::TeleportPhysics);
    if (Controller)
        Controller->SetControlRotation(FRotator(0, Yaw, 0));
    Previous = P;
    bPositionReady = true;
    FootstepDistance = 0;
    BobPhase = 0;
    SprintEffect.Interrupt();
    Camera->SetRelativeLocation({0, 0, 77});
    Camera->SetRelativeRotation(FRotator::ZeroRotator);
}
void ASACharacter::SetBedView(const FVector &Position, const FRotator &Rotation)
{
    Place(Position, Rotation.Yaw);
    bInBed = true;
    BedRotation = Rotation;
    GetCharacterMovement()->DisableMovement();
    SetActorEnableCollision(false);
    Camera->SetRelativeLocation(FVector::ZeroVector);
    if (Controller)
        Controller->SetControlRotation(Rotation);
}
void ASACharacter::Tick(float Delta)
{
    Super::Tick(Delta);
    auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>();
    if (!G || !G->Save)
        return;
    Camera->SetFieldOfView(G->Save->Settings.FOV);
    auto *PC = Cast<APlayerController>(Controller);
    bool Running = PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift));
    GetCharacterMovement()->MaxWalkSpeed = MovementSpeed(G->Content.WalkSpeed, Running);
    FVector Now = GetActorLocation();
    if (!bPositionReady)
    {
        Previous = Now;
        bPositionReady = true;
    }
    if (G->CanMove())
    {
        float D = FVector::Dist2D(Previous, Now);
        SprintEffect.Advance(Delta, D > G->Content.WalkSpeed * 1.45 * Delta);
        FootstepDistance += D;
        BobPhase += D * .07f;
        if (FootstepDistance > (Running ? 120 : 95))
        {
            FootstepDistance = 0;
            if (G->Audio)
                G->Audio->Step();
        }
        G->ObserveMovement(Now);
        float Bob =
            G->Save->Settings.CameraMotion && GetVelocity().SizeSquared2D() > 4 ? FMath::Sin(BobPhase) * 1.2f : 0;
        Camera->SetRelativeLocation({0, 0, 77 + Bob});
    }
    if (!G->CanMove())
    {
        SprintEffect.Interrupt();
        GetCharacterMovement()->StopMovementImmediately();
    }
    Previous = GetActorLocation();
}
void ASAPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ASAPlayerController::Escape);
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ASAPlayerController::Click);
    InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ASAPlayerController::Interact);
    InputComponent->BindKey(EKeys::J, IE_Pressed, this, &ASAPlayerController::PreviousPage);
    InputComponent->BindKey(EKeys::K, IE_Pressed, this, &ASAPlayerController::NextPage);
    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ASAPlayerController::Confirm);
}
void ASAPlayerController::Escape()
{
    if (auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>())
        G->Escape();
}
void ASAPlayerController::Click()
{
    if (auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>())
        if (G->Stage == ESAStage::Hunt || G->Stage == ESAStage::Sleep)
        {
            G->Interact();
            return;
        }
    if (auto *H = Cast<ASAHUD>(GetHUD()))
        H->Click();
}
void ASAPlayerController::Interact()
{
    if (auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>())
        G->Interact();
}
void ASAPlayerController::PreviousPage()
{
    if (auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>())
        G->JournalPage(-1);
}
void ASAPlayerController::NextPage()
{
    if (auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>())
        G->JournalPage(1);
}
void ASAPlayerController::Confirm()
{
    if (auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>())
    {
        if (G->Stage == ESAStage::Menu && !G->bSettings)
        {
            G->NewGame();
            return;
        }
        if (G->Stage == ESAStage::Paused && !G->bSettings)
        {
            G->TogglePause();
            return;
        }
        G->RetrySleep();
    }
}
