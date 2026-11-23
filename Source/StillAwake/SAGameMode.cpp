#include "SAGameMode.h"
#include "SAHouse.h"
#include "SACharacter.h"
#include "SAHUD.h"
#include "SAAudio.h"
#include "SASmokeTest.h"
#include "SAShadowEyes.h"
#include "Camera/CameraComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

namespace
{
struct FCheckpoint
{
    FVector Position;
    const TCHAR *Direction;
};

const FCheckpoint Checkpoints[] = {
    {{150, 0, 88}, TEXT("Walk through the front foyer.")},
    {{935, 235, 88}, TEXT("Check the television in the two-story family room.")},
    {{850, -135, 88}, TEXT("Check the kitchen across from the family room.")},
    {{603, 5, 430}, TEXT("Take the main stairs beside the foyer to the upper landing.")},
    {{800, 0, 430}, TEXT("Follow the gallery overlooking the family room.")},
    {{180, -5, 430}, TEXT("Check the shared bathroom between the front bedrooms.")},
    {{265, -245, 430}, TEXT("Check the child's bedroom at the front of the house.")},
    {{830, -262, 430}, TEXT("Check the bathroom off the main bedroom.")},
};

constexpr uint32 FullRoute = (1u << UE_ARRAY_COUNT(Checkpoints)) - 1;
const FString BedId = TEXT("upstairs.master.bed");

bool IsNearBed(const FVector &Bed, const FVector &Player, const FVector &Eye, const FVector &Forward)
{
    const FVector ToBed = Bed - Eye;
    return FMath::Abs(Player.Z - Bed.Z) < 55 && ToBed.SizeSquared() < FMath::Square(190.0) &&
           FVector::DotProduct(Forward, ToBed.GetSafeNormal()) > .66;
}
}

ASAGameMode::ASAGameMode()
{
    DefaultPawnClass = ASACharacter::StaticClass();
    PlayerControllerClass = ASAPlayerController::StaticClass();
    HUDClass = ASAHUD::StaticClass();
    PrimaryActorTick.bCanEverTick = true;
}

ASACharacter *ASAGameMode::Character() const
{
    return Cast<ASACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

FString ASAGameMode::SaveSlot() const
{
    return FParse::Param(FCommandLine::Get(), TEXT("SmokeTest")) ? TEXT("StillAwakeSmoke") : TEXT("StillAwakeProgress");
}

void ASAGameMode::BeginPlay()
{
    Super::BeginPlay();
    BuildInfo = FSABuildInfo::Load();
    UE_LOG(LogTemp, Display, TEXT("BUILD %s run=%s"), *BuildInfo.BuildVersion, *BuildInfo.RunId);
    if (FParse::Param(FCommandLine::Get(), TEXT("SmokeTest")))
        GetWorld()->SpawnActor<ASASmokeTest>();
    Save = Cast<USASaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot(), 0));
    if (!Save || Save->Version < 1 || Save->Version > 2)
        Save = Cast<USASaveGame>(UGameplayStatics::CreateSaveGameObject(USASaveGame::StaticClass()));
    if (Save->Version == 1)
    {
        Save->BestStreak = 0;
        Save->EndingSeen = false;
        Save->TutorialStep = 0;
        Save->Version = 2;
    }
    Save->Settings.Validate();
    if (!Content.Load(Failure))
    {
        Stage = ESAStage::Error;
        CaptureMouse(false);
        UE_LOG(LogTemp, Error, TEXT("STILL AWAKE: %s"), *Failure);
        return;
    }
    House = GetWorld()->SpawnActor<ASAHouse>();
    House->Build();
    ShadowEyes = GetWorld()->SpawnActor<ASAShadowEyes>();
    for (const auto &A : Content.Anomalies)
    {
        if (House->Props.Contains(A.Target))
            continue;
        Failure = TEXT("Missing anomaly target: ") + A.Target;
        Stage = ESAStage::Error;
        CaptureMouse(false);
        return;
    }
    for (const FString &Id : Content.InspectableTargets)
    {
        if (!House->Props.Contains(Id))
        {
            Failure = TEXT("Missing inspection target: ") + Id;
            Stage = ESAStage::Error;
            CaptureMouse(false);
            return;
        }
        Inspectables.Add(Id);
    }
    Audio = NewObject<USAAudio>(this);
    AddInstanceComponent(Audio);
    Audio->RegisterComponent();
    Title();
}

void ASAGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
    Persist();
    Super::EndPlay(Reason);
}

void ASAGameMode::CaptureMouse(bool Capture)
{
    if (!Capture && ShadowEyes)
        ShadowEyes->Reset();
    auto *PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC)
        return;
    PC->bShowMouseCursor = !Capture;
    PC->bEnableClickEvents = !Capture;
    if (Capture)
    {
        PC->SetInputMode(FInputModeGameOnly());
        return;
    }
    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PC->SetInputMode(Mode);
}

void ASAGameMode::Title()
{
    Stage = ESAStage::Menu;
    bSettings = bJournal = bNewspaper = false;
    Cycle = 0;
    VisitedCheckpoints = 0;
    Hunt.Reset({});
    HuntResult = {};
    Sleep.Reset({});
    House->Reset();
    House->SetCountdown(SA::CycleCount);
    if (Audio)
    {
        Audio->Pause(false);
        Audio->Dawn(false);
        Audio->SetSleepMix(false);
    }
    if (auto *P = Character())
        P->Place(ASAHouse::PorchStart(), 0);
    ApplySettings();
    CaptureMouse(false);
}

void ASAGameMode::NewGame()
{
    Cycle = 0;
    HuntResult = {};
    BeginHunt();
}

void ASAGameMode::Restart()
{
    NewGame();
}

void ASAGameMode::BeginHunt()
{
    if (ShadowEyes)
        ShadowEyes->Reset();
    const auto &Night = Content.Hunts[Cycle];
    House->Reset();
    House->SetCountdown(SA::CycleCount - Cycle);
    bSettings = bJournal = bNewspaper = false;
    VisitedCheckpoints = 0;
    Prompt.Empty();
    Caption.Empty();
    Feedback = Night.StartLine;
    FeedbackRemaining = 5;
    std::vector<bool> Active(Content.InspectableTargets.Num(), false);
    for (int Index : Night.AnomalyIndices)
    {
        const auto &Anomaly = Content.Anomalies[Index];
        Active[Content.InspectableTargets.Find(Anomaly.Target)] = true;
        ApplyAnomaly(Index);
    }
    Hunt.Reset(Active);
    if (auto *P = Character())
        P->Place(ASAHouse::PorchStart(), 0);
    Stage = ESAStage::Hunt;
    Audio->Pause(false);
    Audio->Dawn(false);
    Audio->SetSleepMix(false);
    UnlockStory(Cycle);
    ApplySettings();
    CaptureMouse(true);
    UE_LOG(LogTemp, Display, TEXT("HUNT %d active=%d"), Cycle + 1, Night.AnomalyIndices.Num());
}

void ASAGameMode::ApplyAnomaly(int Index)
{
    const auto &A = Content.Anomalies[Index];
    auto P = House->Props.FindChecked(A.Target);
    switch (A.Effect)
    {
    case 0:
        P->GetRootComponent()->AddLocalOffset(FVector(A.Value.Z, A.Value.X, A.Value.Y) * 100);
        break;
    case 1:
        P->GetRootComponent()->AddLocalRotation(FRotator(A.Value.X, A.Value.Y, A.Value.Z));
        break;
    case 2:
        P->SetActorRelativeScale3D(P->GetRootComponent()->GetRelativeScale3D() *
                                   FVector(A.Value.Z, A.Value.X, A.Value.Y));
        break;
    case 3:
    case 5:
        P->Visible(A.Enabled);
        break;
    case 6:
        P->Tint(FLinearColor(A.Value.X, A.Value.Y, A.Value.Z));
        break;
    case 4:
        if (P->Words)
            P->Words->SetText(FText::FromString(A.Text));
        break;
    }
}

void ASAGameMode::ObserveMovement(const FVector &Position)
{
    if (!CanMove())
        return;
    for (int I = 0; I < UE_ARRAY_COUNT(Checkpoints); ++I)
    {
        const FVector To = Position - Checkpoints[I].Position;
        if (FMath::Abs(To.Z) < 50 && To.SizeSquared2D() < FMath::Square(125.0))
            VisitedCheckpoints |= 1u << I;
    }
}

bool ASAGameMode::RouteComplete() const
{
    return VisitedCheckpoints == FullRoute;
}

void ASAGameMode::FinishHunt()
{
    HuntResult = Hunt.Result();
    SleepProfile = Content.SleepProfiles[static_cast<int>(HuntResult.Difficulty)];
    const double Pressure = Content.Hunts[Cycle].SleepPressure;
    SleepProfile.AnxietyRiseRate *= Pressure;
    SleepProfile.ClosetAdvanceRate *= Pressure;
    Stage = ESAStage::HuntResult;
    PhaseRemaining = 2.8f;
    Prompt.Empty();
    Caption.Empty();
    Feedback.Empty();
    Persist();
    UE_LOG(LogTemp, Display, TEXT("HUNT %d correct=%d missed=%d false=%d raw=%.2f performance=%.2f difficulty=%d"),
           Cycle + 1, HuntResult.Correct, HuntResult.Missed, HuntResult.FalsePositives, HuntResult.Raw,
           HuntResult.Performance, static_cast<int>(HuntResult.Difficulty));
}

void ASAGameMode::BeginSleep()
{
    if (ShadowEyes)
        ShadowEyes->Reset();
    Sleep.Reset(SleepProfile);
    House->Reset();
    House->SetCountdown(SA::CycleCount - Cycle);
    House->SetSleepScene(true, Sleep.IsLampOn(), 0, Save->Settings.ReducedIntensity);
    if (auto *P = Character())
        P->SetBedView(House->BedViewLocation(), ASAHouse::BedViewRotation());
    Stage = ESAStage::Sleep;
    Feedback = Content.Hunts[Cycle].SleepLine;
    FeedbackRemaining = 4;
    Audio->Pause(false);
    Audio->SetSleepMix(true);
    CaptureMouse(true);
}

void ASAGameMode::RetrySleep()
{
    if (Stage == ESAStage::SleepFailed)
        BeginSleep();
}

void ASAGameMode::UpdateSleep(float Delta)
{
    const bool Warned = Sleep.IsWarning();
    Sleep.Advance(Delta);
    House->SetSleepScene(true, Sleep.IsLampOn(), Sleep.Closet(), Save->Settings.ReducedIntensity, Delta);
    if (!Warned && Sleep.IsWarning())
        Audio->Door();
    const auto Outcome = Sleep.Outcome();
    if (Outcome == SA::SleepOutcome::InProgress)
        return;
    UE_LOG(LogTemp, Display, TEXT("SLEEP %d outcome=%d progress=%.2f"), Cycle + 1, static_cast<int>(Outcome),
           Sleep.Progress());
    if (Outcome == SA::SleepOutcome::Asleep)
    {
        if (Cycle + 1 == SA::CycleCount)
        {
            BeginEnding();
            return;
        }
        ++Cycle;
        HuntResult = {};
        BeginHunt();
        return;
    }
    Stage = ESAStage::SleepFailed;
    CaptureMouse(false);
}

void ASAGameMode::Tick(float Delta)
{
    Super::Tick(Delta);
    if (Stage == ESAStage::Error || !Audio)
        return;
    UpdateShadowEyes(Delta);
    Audio->Apply(Save->Settings, Cycle);
    if (Stage == ESAStage::HuntResult)
    {
        PhaseRemaining -= Delta;
        if (PhaseRemaining <= 0)
            BeginSleep();
        return;
    }
    if (Stage == ESAStage::Sleep)
    {
        FeedbackRemaining = FMath::Max(0.f, FeedbackRemaining - Delta);
        if (FeedbackRemaining == 0)
            Feedback.Empty();
        UpdateSleep(Delta);
        return;
    }
    if (Stage == ESAStage::Ending)
        EndingAge += Delta;
    if (Stage != ESAStage::Hunt)
    {
        Audio->Anomalies(0, 0);
        return;
    }
    House->Animate(Delta);
    const auto *P = Character();
    if (P)
    {
        FVector Travel = P->GetPendingMovementInputVector();
        if (Travel.IsNearlyZero())
            Travel = P->GetLastMovementInputVector();
        if (CanMove() && House->UpdateDoors(Delta, P, Travel))
            Audio->Door();
        const auto Gain = [this, P](const FString &Id, float Radius)
        {
            const auto *Prop = House->Props.FindChecked(Id).Get();
            return Prop->IsHidden() ? 0.0 : FMath::Clamp(1 - FVector::Dist(P->GetActorLocation(),
                                                           Prop->GetActorLocation()) / Radius, 0.0, 1.0);
        };
        const float Water = Gain(TEXT("kitchen.water"), 1100);
        const float TV = Gain(TEXT("living.static"), 800);
        Audio->Anomalies(Water, TV);
        Caption.Empty();
        if (Water > .64f)
            Caption = TEXT("[Water running]");
        if (TV > .65f && TV > Water)
            Caption = TEXT("[Television static]");
    }
    FeedbackRemaining = FMath::Max(0.f, FeedbackRemaining - Delta);
    if (FeedbackRemaining == 0)
        Feedback.Empty();
    UpdateHuntPrompt();
}

void ASAGameMode::UpdateShadowEyes(float Delta)
{
    if (!ShadowEyes)
        return;
    const auto *Player = Character();
    const auto *Viewport = GetWorld()->GetGameViewport();
    if (!CanLook() || !Player || !Viewport || !Viewport->Viewport || !Viewport->Viewport->HasFocus() ||
        Player->Glimpse(false).Blackout > .05)
    {
        ShadowEyes->Reset();
        return;
    }
    ShadowEyes->Observe(House, Player->Camera->GetComponentLocation(), Player->Camera->GetForwardVector(), Player, Delta);
}

void ASAGameMode::UpdateHuntPrompt()
{
    Prompt.Empty();
    if (!CanMove())
        return;
    const FString Id = InteractionTarget();
    if (Id == TEXT("porch.newspaper"))
    {
        Prompt = TEXT("E / Click  Read the note");
        return;
    }
    if (Id == TEXT("foyer.book"))
    {
        Prompt = TEXT("E / Click  Read the journal");
        return;
    }
    if (Id == BedId)
    {
        Prompt = RouteComplete() ? TEXT("E / Click  Finish checking and get into bed") : TEXT("Finish checking the house first.");
        return;
    }
    if (Id.EndsWith(TEXT(".door")))
    {
        Prompt = House->ShouldCloseDoor(Id) ? TEXT("E / Click  Close door") : TEXT("Walk into the door to push it");
        return;
    }
    const int Index = Content.InspectableTargets.Find(Id);
    if (Index != INDEX_NONE)
        Prompt = Hunt.IsMarked(Index) ? TEXT("Marked") : TEXT("E / Click  Apply Fix it sticker");
}

FString ASAGameMode::InteractionTarget(FVector *StickerPoint) const
{
    auto *P = Character();
    if (!P)
        return {};
    const FVector Start = P->Camera->GetComponentLocation();
    const FVector Forward = P->Camera->GetForwardVector();
    FCollisionQueryParams Query;
    Query.AddIgnoredActor(P);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Forward * 320, ECC_Visibility, Query))
    {
        if (const auto *Actor = Hit.GetActor())
        {
            for (const FName &Tag : Actor->Tags)
            {
                const FString Id = Tag.ToString();
                if (Id == TEXT("porch.newspaper") || Id == TEXT("foyer.book") ||
                    (Id.EndsWith(TEXT(".door")) && House->Props.Contains(Id)))
                    return Id;
            }
        }
    }
    const FString Inspection = House->FindInspectable(Start, Forward, Inspectables, P, StickerPoint);
    if (!Inspection.IsEmpty())
        return Inspection;
    if (IsNearBed(House->BedInteractionLocation(), P->GetActorLocation(), Start, Forward))
        return BedId;
    return {};
}

void ASAGameMode::TogglePause()
{
    if (Stage == ESAStage::Paused)
    {
        Stage = BeforePause;
        bSettings = false;
        Audio->Pause(false);
        CaptureMouse(true);
        return;
    }
    if (Stage == ESAStage::Hunt || Stage == ESAStage::HuntResult || Stage == ESAStage::Sleep)
    {
        BeforePause = Stage;
        Stage = ESAStage::Paused;
        bJournal = bNewspaper = false;
        Audio->Pause(true);
        CaptureMouse(false);
    }
}

void ASAGameMode::Escape()
{
    if (bSettings)
    {
        CloseSettings();
        return;
    }
    if (bNewspaper || bJournal)
    {
        bNewspaper = bJournal = false;
        return;
    }
    TogglePause();
}

void ASAGameMode::Settings()
{
    bSettings = true;
    CaptureMouse(false);
}

void ASAGameMode::CloseSettings()
{
    bSettings = false;
    Persist();
}

void ASAGameMode::QuitGame()
{
    Persist();
    UKismetSystemLibrary::QuitGame(this, UGameplayStatics::GetPlayerController(this, 0), EQuitPreference::Quit, false);
}

void ASAGameMode::Interact()
{
    if (Stage == ESAStage::Sleep)
    {
        Sleep.ToggleLamp();
        House->SetSleepScene(true, Sleep.IsLampOn(), Sleep.Closet(), Save->Settings.ReducedIntensity);
        Audio->Lamp();
        return;
    }
    if (Stage != ESAStage::Hunt)
        return;
    if (bNewspaper || bJournal)
    {
        bNewspaper = bJournal = false;
        return;
    }
    FVector StickerPoint = FVector::ZeroVector;
    const FString Id = InteractionTarget(&StickerPoint);
    if (Id == TEXT("porch.newspaper"))
    {
        bNewspaper = true;
        return;
    }
    if (Id == TEXT("foyer.book"))
    {
        bJournal = true;
        JournalIndex = FMath::Max(0, UnlockedStories().Num() - 1);
        return;
    }
    if (Id.EndsWith(TEXT(".door")))
    {
        House->ToggleDoor(Id, Character()->GetActorLocation());
        Audio->Door();
        return;
    }
    if (Id == BedId)
    {
        if (RouteComplete())
            FinishHunt();
        return;
    }
    const auto Judgment = Hunt.Mark(Content.InspectableTargets.Find(Id));
    if (Judgment == SA::Judgment::Rejected)
        return;
    House->AddSticker(Id, StickerPoint, Character()->Camera->GetComponentLocation());
    Feedback = Judgment == SA::Judgment::Correct ? TEXT("Something was out of place.") : TEXT("That belongs here.");
    FeedbackRemaining = 2.5f;
}

FString ASAGameMode::NewspaperText()
{
    return TEXT("Check downstairs, then go up. Put a Fix it sticker on anything that seems out of place. "
                "Leave things that look right alone.\n\n"
                "Once you've seen both floors, go back to bed. The lamp drives back the shape in the closet, "
                "but the light makes it harder to settle. Switch it off when you can.");
}

void ASAGameMode::JournalPage(int Direction)
{
    if (bJournal && Stage == ESAStage::Hunt)
        JournalIndex = FMath::Clamp(JournalIndex + Direction, 0, FMath::Max(0, UnlockedStories().Num() - 1));
}

TArray<FSAStory> ASAGameMode::UnlockedStories() const
{
    TArray<FSAStory> Result;
    for (const auto &S : Content.Stories)
        if (Save->Unlocked.Contains(S.Id))
            Result.Add(S);
    return Result;
}

void ASAGameMode::UnlockStory(int CompletedCycles)
{
    for (const auto &S : Content.Stories)
        if (CompletedCycles >= S.Unlock)
            Save->Unlocked.AddUnique(S.Id);
}

void ASAGameMode::Persist()
{
    if (!Save)
        return;
    Save->Settings.Validate();
    if (!UGameplayStatics::SaveGameToSlot(Save, SaveSlot(), 0))
        UE_LOG(LogTemp, Warning, TEXT("Progress could not be saved; current session can continue."));
}

void ASAGameMode::ApplySettings()
{
    if (!Save)
        return;
    Save->Settings.Validate();
    if (House)
        House->ApplyBrightness(Save->Settings.Brightness);
    if (Audio)
        Audio->Apply(Save->Settings, Cycle);
}

void ASAGameMode::BeginEnding()
{
    Stage = ESAStage::Ending;
    Save->EndingSeen = true;
    UnlockStory(SA::CycleCount);
    Persist();
    EndingAge = 0;
    House->SetSleepScene(false, false, 0);
    House->ApplyBrightness(Save->Settings.Brightness, true);
    Audio->Dawn(true);
    CaptureMouse(false);
}

FString ASAGameMode::Guidance() const
{
    if (!CanMove())
        return {};
    for (int I = 0; I < UE_ARRAY_COUNT(Checkpoints); ++I)
        if (!(VisitedCheckpoints & (1u << I)))
            return Checkpoints[I].Direction;
    return TEXT("Return to the main bedroom and face the bed.");
}
