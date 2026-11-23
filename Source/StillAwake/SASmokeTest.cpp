#include "SASmokeTest.h"
#include "SAGameMode.h"
#include "SAHouse.h"
#include "SACharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "SAData.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"

ASASmokeTest::ASASmokeTest()
{
    PrimaryActorTick.bCanEverTick = true;
}
void ASASmokeTest::Capture(const TCHAR *Name)
{
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Smoke") / (FString(Name) + TEXT(".png")),
                                          true, false);
}
void ASASmokeTest::Finish(bool Success, const FString &Detail)
{
    bComplete = true;
    FString ReportPath = FPaths::ProjectSavedDir() / TEXT("Smoke/result.json");
    FParse::Value(FCommandLine::Get(), TEXT("SmokeReport="), ReportPath);
    const FString Directory = FPaths::GetPath(ReportPath);
    IFileManager::Get().MakeDirectory(*Directory, true);
    auto Object = MakeShared<FJsonObject>();
    Object->SetBoolField(TEXT("passed"), Success);
    Object->SetNumberField(TEXT("checks"), Checks);
    Object->SetStringField(TEXT("detail"), Detail);
    if (Game)
    {
        Object->SetStringField(TEXT("version"), Game->BuildInfo.Version);
        Object->SetStringField(TEXT("buildNumber"), Game->BuildInfo.BuildNumber);
        Object->SetStringField(TEXT("buildVersion"), Game->BuildInfo.BuildVersion);
        Object->SetStringField(TEXT("runId"), Game->BuildInfo.RunId);
        Object->SetStringField(TEXT("configuration"), Game->BuildInfo.Configuration);
    }
    FString Report;
    FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Report));
    FFileHelper::SaveStringToFile(Report, *ReportPath);
    UE_LOG(LogTemp, Display, TEXT("SMOKE %s: %s (%d checks)"), Success ? TEXT("PASS") : TEXT("FAIL"), *Detail, Checks);
    FPlatformMisc::RequestExitWithStatus(false, Success ? 0 : 1);
}
void ASASmokeTest::AimAt(const FVector &Target) const
{
    auto *Player = Game->Character();
    const FRotator Look = (Target - Player->Camera->GetComponentLocation()).Rotation();
    Player->Controller->SetControlRotation(Look);
    Player->Camera->SetWorldRotation(Look);
}

bool ASASmokeTest::ObserveFrontDoor(float Delta)
{
    const auto *Door = Game->House->Props.FindChecked(TEXT("front.door")).Get();
    const double Angle = FMath::FindDeltaAngleDegrees(Door->GetActorRotation().Yaw, Door->Baseline.Rotator().Yaw);
    if (Angle < -.5)
    {
        Finish(false, TEXT("The front door swung toward the player approaching from the porch"));
        return false;
    }
    if (PreviousFrontDoorAngle >= 1 && Angle <= 45 && Angle > PreviousFrontDoorAngle + .001)
        FrontDoorTravelTime += Delta;
    if (Scenario == 0 && PreviousFrontDoorAngle < 35 && Angle >= 35)
        Capture(TEXT("02-door-push"));
    PreviousFrontDoorAngle = Angle;
    return true;
}

bool ASASmokeTest::VerifyDoorPassage(const FString &Id)
{
    if (!Game->House->DoorOpen(Id))
    {
        Finish(false, TEXT("Walking through the doorway did not open ") + Id);
        return false;
    }
    if (Id == TEXT("front.door"))
    {
        if (FrontDoorTravelTime < .15f)
        {
            Finish(false, TEXT("The front door did not visibly swing over time during approach"));
            return false;
        }
        bPassedFrontDoor = true;
        Game->Content.WalkSpeed = 425;
    }
    ++Checks;
    return true;
}

bool ASASmokeTest::PlaceSticker(const FString &Id, const FVector &Target)
{
    AimAt(Target);
    const FString Focus = Game->InteractionTarget();
    if (Focus != Id)
    {
        Finish(false, FString::Printf(TEXT("Expected focus %s but found %s at %s"), *Id, *Focus,
                                      *Game->Character()->GetActorLocation().ToString()));
        return false;
    }
    const int Index = Game->Content.InspectableTargets.Find(Id);
    Game->Interact();
    if (!Game->Hunt.IsMarked(Index))
    {
        Finish(false, TEXT("Player interaction failed to mark ") + Id);
        return false;
    }
    ++Checks;
    const auto Before = Game->Hunt.Result();
    Game->Interact();
    const auto After = Game->Hunt.Result();
    if (Before.Correct != After.Correct || Before.FalsePositives != After.FalsePositives)
    {
        Finish(false, TEXT("A repeated sticker changed the result for ") + Id);
        return false;
    }
    ++Checks;
    return true;
}

void ASASmokeTest::CheckAnomaliesAndStart()
{
    const auto *Figures = LoadObject<UTexture2D>(nullptr, TEXT("/Game/StillAwake/Art/T_PeripheralHorrors.T_PeripheralHorrors"));
    if (!Figures || !Game->House->Surface || Game->House->Surface->GetFName() != TEXT("M_HouseTextured"))
    {
        Finish(false, TEXT("The runtime could not load the textured house and peripheral figures"));
        return;
    }
    ++Checks;
    if (!FParse::Param(FCommandLine::Get(), TEXT("nullrhi")))
    {
        const auto *Atlas = LoadObject<UTexture2D>(nullptr, TEXT("/Game/StillAwake/Art/T_HouseSurfaces.T_HouseSurfaces"));
        const int32 Mips = Atlas ? Atlas->GetNumMips() : 0;
        if (Mips <= 1 || !Figures->HasAlphaChannel())
        {
            Finish(false, FString::Printf(TEXT("Rendered textures need atlas mipmaps and sprite alpha (mips=%d, alpha=%d)"),
                                          Mips, Figures->HasAlphaChannel()));
            return;
        }
        ++Checks;
    }
    for (int I = 0; I < Game->Content.Anomalies.Num(); ++I)
    {
        Game->House->Reset();
        const auto &A = Game->Content.Anomalies[I];
        auto *P = Game->House->Props.FindChecked(A.Target).Get();
        Game->ApplyAnomaly(I);
        const auto MatchesBaseline = [P]()
        {
            return P->GetRootComponent()->GetRelativeTransform().Equals(P->Baseline) &&
                   P->IsHidden() != P->bBaselineVisible && !P->bTinted &&
                   (!P->Words || P->Words->Text.ToString() == P->BaselineText);
        };
        if (MatchesBaseline())
        {
            Finish(false, TEXT("Anomaly has no effect: ") + A.Id);
            return;
        }
        ++Checks;
        Game->House->Reset();
        if (!MatchesBaseline())
        {
            Finish(false, TEXT("Anomaly reset failed: ") + A.Id);
            return;
        }
        ++Checks;
    }

    Game->Save->EndingSeen = false;
    Game->NewGame();
    if (Game->Stage != ESAStage::Hunt || Game->Cycle != 0 || Game->RouteComplete() ||
        !Game->Character()->GetActorLocation().Equals(ASAHouse::PorchStart(), 5))
    {
        Finish(false, TEXT("New game did not start a fresh first hunt on the porch"));
        return;
    }
    ++Checks;
    AimAt(Game->House->Props.FindChecked(TEXT("porch.newspaper"))->GetActorLocation());
    Game->Interact();
    if (!Game->bNewspaper || Game->CanMove())
    {
        Finish(false, TEXT("The porch note did not open or stop movement"));
        return;
    }
    ++Checks;
    Game->Escape();
    if (Game->bNewspaper || !Game->CanMove() || Game->Content.WalkSpeed != 300 ||
        ASACharacter::MovementSpeed(300, true) != 600)
    {
        Finish(false, TEXT("Closing the porch note did not restore expected movement"));
        return;
    }
    ++Checks;
    Game->TogglePause();
    if (Game->Stage != ESAStage::Paused || Game->CanMove() || Game->CanLook())
    {
        Finish(false, TEXT("Pause did not stop hunt movement and look"));
        return;
    }
    ++Checks;
    Game->TogglePause();
    Capture(TEXT("02-porch"));
}

void ASASmokeTest::BeginTraversal()
{
    Scenario = Game->Cycle;
    ScenarioAge = 0;
    Waypoint = 0;
    StalledTime = 0;
    LastProgressPosition = Game->Character()->GetActorLocation();
    FrontDoorTravelTime = 0;
    PreviousFrontDoorAngle = 0;
    bPassedFrontDoor = false;
    Game->Content.WalkSpeed = 300;
    const auto *FrontDoor = Game->House->Props.FindChecked(TEXT("front.door")).Get();
    if (!FrontDoor->GetActorRotation().Equals(FrontDoor->Baseline.Rotator(), .05))
    {
        Finish(false, TEXT("The front door was open before the player approached it"));
        return;
    }
    const TCHAR *FoyerMark = Scenario != 1 ? TEXT("foyer.clock") : nullptr;
    const TCHAR *CoatMark = Scenario == 1 ? TEXT("foyer.raincoat") : nullptr;
    const TCHAR *TVMark = Scenario == 0 ? TEXT("living.static") : nullptr;
    const TCHAR *TowelMark = Scenario == 0 ? TEXT("upstairs.bath.towel") : nullptr;
    const TCHAR *DollMark = Scenario == 1 ? TEXT("upstairs.child.doll") : nullptr;
    const TCHAR *PhotoMark = Scenario == 2 ? TEXT("upstairs.master.photo") : nullptr;
    Waypoints = {
        {{-95, 0, 88}},
        {{150, 0, 88}, TEXT("front.door"), CoatMark, nullptr, {150, 98, 135}},
    };
    if (Scenario == 0)
    {
        Waypoints.Append({{{132, 0, 88}}, {{132, -205, 88}}, {{132, 0, 88}},
                          {{298, 0, 88}}, {{298, 240, 88}}, {{298, 0, 88}}});
    }
    Waypoints.Append({
        {{336, 0, 88}, nullptr, FoyerMark, nullptr, {336, -95, 118}},
        {{470, 0, 88}}, {{490, -70, 88}}, {{680, -70, 88}}, {{850, -135, 88}}, {{935, -85, 88}},
        {{935, 235, 88}, nullptr, TVMark, nullptr, {1090, 235, 155}},
    });
    if (Scenario == 0)
    {
        // Exercise the deck, rear entry, garage and service stair in addition to the hunt route.
        Waypoints.Append({
            {{935, 120, 88}, nullptr, nullptr, TEXT("04-family"), {858, 295, 454}},
            {{935, 60, 88}}, {{1060, 60, 88}}, {{1190, 60, 88}, TEXT("back.door")}, {{1345, 215, 88}},
            {{1190, 60, 88}}, {{1060, 60, 88}}, {{935, 60, 88}}, {{935, -85, 88}},
            {{1060, -185, 88}}, {{1220, -185, 88}}, {{1220, -105, 88}}, {{1450, -105, 88}}, {{1470, -402, 88}},
            {{1665, -409, 88}}, {{1678, -459, 88}},
            {{1678, -573, 88}, TEXT("laundry.door")}, {{1678, -459, 88}}, {{1810, -479, 88}},
            {{1993, -489, 88}, TEXT("garage.entry.door")}, {{1993, -572, 88}},
            {{2200, -572, 88}, nullptr, nullptr, TEXT("05-garage"), {2370, -343, 96}},
            {{1993, -572, 88}}, {{1993, -489, 88}}, {{1810, -479, 88}},
            {{1665, -409, 88}}, {{1470, -402, 88}}, {{1450, -116, 88}},
            {{1515, -116, 88}}, {{1757, -116, 259}}, {{1808, -116, 259}},
            {{1808, -501, 430}}, {{1920, -501, 430}, TEXT("upstairs.rear.door")},
            {{1980, -501, 430}},
            {{2170, -500, 430}, TEXT("upstairs.media.door"), nullptr, TEXT("06-media"), {2410, -400, 478}},
            {{1991, -501, 430}}, {{1991, -605, 430}}, {{1950, -605, 430}},
            {{1950, -704, 430}, TEXT("upstairs.rear.bath.door")},
            {{1950, -605, 430}}, {{1991, -605, 430}}, {{1991, -501, 430}}, {{1808, -501, 430}}, {{1590, -500, 430}},
            {{1590, -282, 430}}, {{1448, -282, 430}, TEXT("upstairs.master.rear.door")},
            {{1590, -282, 430}}, {{1590, -500, 430}}, {{1808, -501, 430}},
            {{1808, -116, 259}}, {{1757, -116, 259}}, {{1515, -116, 88}}, {{1450, -105, 88}},
            {{1220, -105, 88}}, {{1220, -185, 88}}, {{1060, -185, 88}}, {{935, -85, 88}},
        });
    }
    if (Scenario != 0)
        Waypoints.Add({{935, -85, 88}});
    Waypoints.Append({
        {{680, -70, 88}}, {{493, -40, 88}},
        {{493, 8, 88}, nullptr, nullptr, Scenario == 0 ? TEXT("07-stairs") : nullptr, {493, 260, 218}},
        {{493, 379, 259}}, {{603, 379, 259}}, {{603, -15, 430}},
        {{800, 0, 430}, nullptr, nullptr, Scenario == 0 ? TEXT("08-gallery") : nullptr, {948, 255, 255}},
        {{815, -63, 430}}, {{850, -63, 430}},
        {{1004, -63, 430}, TEXT("upstairs.master.door")},
        {{1280, -70, 430}, nullptr, PhotoMark, nullptr, {1369, -332, 422}, true},
        {{1100, -63, 430}}, {{1100, -262, 430}}, {{1004, -262, 430}},
        {{830, -262, 430}, TEXT("upstairs.ensuite.door"), nullptr,
         Scenario == 0 ? TEXT("09-ensuite") : nullptr, {800, -360, 430}},
        {{1004, -262, 430}}, {{1100, -262, 430}}, {{1100, -63, 430}}, {{1004, -63, 430}},
        {{815, -63, 430}}, {{800, 0, 430}}, {{603, -15, 430}},
        {{365, -5, 430}}, {{365, -75, 430}}, {{365, -240, 430}, TEXT("upstairs.child.door")},
        {{300, -300, 430}, nullptr, DollMark, nullptr, {377, -385, 463}},
        {{365, -240, 430}}, {{365, -5, 430}}, {{310, -5, 430}},
        {{180, -5, 430}, TEXT("upstairs.bath.door"), TowelMark, nullptr, {120, 119, 442}}, {{365, -5, 430}},
    });
    if (Scenario == 0)
    {
        Waypoints.Append({{{365, 65, 430}}, {{365, 240, 430}, TEXT("upstairs.third.door")}, {{365, -5, 430}}});
    }
    Waypoints.Append({{{603, -15, 430}}, {{800, 0, 430}}, {{815, -63, 430}}, {{1004, -63, 430}},
                      {{1280, -70, 430}, nullptr, nullptr, nullptr, {}, true}});
}

void ASASmokeTest::FollowRoute(float Delta)
{
    auto *Player = Game->Character();
    if (!bPassedFrontDoor && !ObserveFrontDoor(Delta))
        return;
    if (ScenarioAge > 85 || !Waypoints.IsValidIndex(Waypoint))
    {
        const FString Destination = Waypoints.IsValidIndex(Waypoint) ? Waypoints[Waypoint].Position.ToString() : TEXT("none");
        Finish(false, FString::Printf(TEXT("Hunt %d blocked at waypoint %d (%s), player position %s"), Scenario + 1,
                                      Waypoint, *Destination, *Player->GetActorLocation().ToString()));
        return;
    }
    const auto &Step = Waypoints[Waypoint];
    const FVector Position = Player->GetActorLocation();
    FVector To = Step.Position - Position;
    To.Z = 0;
    if (To.SizeSquared() >= FMath::Square(12.0))
    {
        if (FVector::DistSquared2D(Position, LastProgressPosition) > FMath::Square(5.0))
        {
            LastProgressPosition = Position;
            StalledTime = 0;
        }
        StalledTime += Delta;
        const FVector Direction = To.GetSafeNormal();
        if (StalledTime >= 3)
        {
            const auto *Capsule = Player->GetCapsuleComponent();
            const FCollisionQueryParams Query(SCENE_QUERY_STAT(SmokeRouteObstacle), false, Player);
            FHitResult Hit;
            GetWorld()->SweepSingleByProfile(Hit, Position, Position + Direction * 64, FQuat::Identity,
                                             Capsule->GetCollisionProfileName(), Capsule->GetCollisionShape(-.5f), Query);
            const AActor *Obstacle = Hit.GetActor();
            const FString Blocker = Obstacle ? (Obstacle->Tags.IsEmpty() ? Obstacle->GetName() : Obstacle->Tags[0].ToString())
                                             : TEXT("none found");
            Finish(false, FString::Printf(TEXT("Hunt %d stalled at waypoint %d (%s), position %s, blocker %s"),
                                          Scenario + 1, Waypoint, *Step.Position.ToString(), *Position.ToString(), *Blocker));
            return;
        }
        Player->Controller->SetControlRotation(To.Rotation());
        Player->AddMovementInput(Direction, 1);
        return;
    }

    StalledTime = 0;
    LastProgressPosition = Position;
    Player->GetCharacterMovement()->StopMovementImmediately();
    if (FMath::Abs(Player->GetActorLocation().Z - Step.Position.Z) > 18)
    {
        Finish(false, FString::Printf(TEXT("Wrong floor at waypoint %d: %s"), Waypoint,
                                      *Player->GetActorLocation().ToString()));
        return;
    }
    ++Checks;
    if (Step.PassedDoor && !VerifyDoorPassage(Step.PassedDoor))
        return;
    if (Step.Inspection && !PlaceSticker(Step.Inspection, Step.View))
        return;
    if (Step.Image)
    {
        AimAt(Step.View);
        PendingImage = Step.Image;
        Wait = .22f;
    }
    const bool bLast = ++Waypoint == Waypoints.Num();
    if (!Step.bUseBed)
        return;
    AimAt(Game->House->BedInteractionLocation());
    if (Game->InteractionTarget() != TEXT("upstairs.master.bed") || Game->RouteComplete() != bLast)
    {
        Finish(false, TEXT("Bed focus or physical route completion was incorrect"));
        return;
    }
    ++Checks;
    Game->Interact();
    if (Game->Stage != (bLast ? ESAStage::HuntResult : ESAStage::Hunt))
    {
        Finish(false, TEXT("Bed interaction began at the wrong point in the hunt"));
        return;
    }
    ++Checks;
}

void ASASmokeTest::TrySleep()
{
    const int Correct = Scenario < 2 ? 2 : 1;
    const int FalsePositives = Scenario != 1 ? 1 : 0;
    const int Total = Game->Content.Hunts[Scenario].AnomalyIndices.Num();
    const auto ExpectedDifficulty = Scenario == 0 ? SA::SleepDifficulty::Severe
                                  : Scenario == 1 ? SA::SleepDifficulty::Pressured : SA::SleepDifficulty::Extreme;
    const auto Result = Game->HuntResult;
    if (Game->Cycle != Scenario || Result.Correct != Correct || Result.Missed != Total - Correct ||
        Result.FalsePositives != FalsePositives || Result.Difficulty != ExpectedDifficulty)
    {
        Finish(false, FString::Printf(TEXT("Hunt %d scored or selected difficulty incorrectly: %d/%d/%d, %.2f"),
                                      Scenario + 1, Result.Correct, Result.Missed, Result.FalsePositives, Result.Performance));
        return;
    }
    ++Checks;
    if (Game->CanMove() || !Game->CanLook() || !Game->Character()->Camera->GetComponentLocation().Equals(
                                                   Game->House->BedViewLocation(), 1))
    {
        Finish(false, TEXT("Sleep did not constrain the character to the first-person bed view"));
        return;
    }
    ++Checks;

    const auto &Profile = Game->Content.SleepProfiles[static_cast<int>(ExpectedDifficulty)];
    const double ExpectedAdvance = Profile.ClosetAdvanceRate * Game->Content.Hunts[Scenario].SleepPressure /
                                   Profile.ClosetFailureThreshold;
    const double Before = Game->Sleep.Closet();
    Game->Tick(.4f);
    if (!FMath::IsNearlyEqual(Game->Sleep.Closet() - Before, .4 * ExpectedAdvance, .0001))
    {
        Finish(false, TEXT("The sleep rates did not match this hunt's selected difficulty and pressure"));
        return;
    }
    ++Checks;
    const double Threat = Game->Sleep.Closet();
    Game->TogglePause();
    Game->Tick(.4f);
    if (Game->Stage != ESAStage::Paused || Game->Sleep.Closet() != Threat)
    {
        Finish(false, TEXT("Pause did not freeze the sleep system"));
        return;
    }
    ++Checks;
    Game->TogglePause();

    if (Scenario == 0)
    {
        Game->Tick(static_cast<float>((1 - Threat) / ExpectedAdvance - .3));
        if (!Game->Sleep.IsWarning() || Game->Stage != ESAStage::Sleep)
        {
            Finish(false, TEXT("No warning before the closet failure"));
            return;
        }
        ++Checks;
        Game->Tick(.4f);
        if (Game->Stage != ESAStage::SleepFailed || Game->Sleep.Outcome() != SA::SleepOutcome::Presence)
        {
            Finish(false, TEXT("Ignoring the closet warning did not enter the failure state"));
            return;
        }
        ++Checks;
        Game->RetrySleep();
        if (Game->Stage != ESAStage::Sleep || Game->Cycle != Scenario || Game->Sleep.IsLampOn() ||
            Game->Sleep.Progress() != 0 || Game->Sleep.Closet() != 0 || Game->HuntResult.Performance != Result.Performance ||
            Game->HuntResult.Difficulty != ExpectedDifficulty)
        {
            Finish(false, TEXT("Retry did not reset bedside state and preserve the scored hunt"));
            return;
        }
        ++Checks;
    }

    for (int Step = 0; Step < 2400 && Game->Stage == ESAStage::Sleep; ++Step)
    {
        const bool bOn = Game->Sleep.IsLampOn();
        if ((!bOn && Game->Sleep.Closet() >= .75) || (bOn && Game->Sleep.Closet() <= .45))
        {
            Game->Interact();
            if (Game->Sleep.IsLampOn() == bOn)
            {
                Finish(false, TEXT("The player interaction failed to toggle the lamp"));
                return;
            }
        }
        Game->Tick(.05f);
    }
    if (Scenario + 1 == SA::CycleCount && Game->Stage == ESAStage::Ending && Game->Sleep.Outcome() == SA::SleepOutcome::Asleep)
    {
        ++Checks;
        return;
    }
    if (Game->Stage != ESAStage::Hunt || Game->Cycle != Scenario + 1 || Game->RouteComplete() ||
        Game->Hunt.Result().Correct != 0 || Game->Hunt.Result().FalsePositives != 0)
    {
        Finish(false, FString::Printf(TEXT("Sleep %d did not start the next phase"), Scenario + 1));
        return;
    }
    ++Checks;
}

void ASASmokeTest::VerifyEnding()
{
    if (Game->EndingAge < 2)
        return;
    if (bEndingCaptured)
    {
        Finish(true, TEXT("Completed all three physical house routes and paired sleeps, including sticker judgments, "
                          "pause, closet warning, retry, ending, and save reload"));
        return;
    }
    if (Scenario + 1 != SA::CycleCount)
    {
        Finish(false, TEXT("Ending appeared before the third sleep"));
        return;
    }
    ++Checks;
    const auto *Saved = Cast<USASaveGame>(UGameplayStatics::LoadGameFromSlot(Game->SaveSlot(), 0));
    if (!Saved || Saved->Version != 2 || !Saved->EndingSeen)
    {
        Finish(false, TEXT("Completed ending did not persist to the smoke profile"));
        return;
    }
    ++Checks;
    bEndingCaptured = true;
    Capture(TEXT("06-ending"));
    Wait = .25f;
}

void ASASmokeTest::Tick(float Delta)
{
    Super::Tick(Delta);
    if (bComplete)
        return;
    Age += Delta;
    ScenarioAge += Delta;
    if (Age > 360)
    {
        Finish(false, TEXT("Integration test timed out"));
        return;
    }
    if (!Game)
    {
        Game = GetWorld()->GetAuthGameMode<ASAGameMode>();
        if (!Game || !Game->Character())
        {
            Game = nullptr;
            return;
        }
        Capture(TEXT("01-title"));
    }
    if (Game->Stage == ESAStage::Error)
    {
        Finish(false, Game->Failure);
        return;
    }
    if (Wait > 0)
    {
        if (PendingImage)
        {
            Capture(PendingImage);
            PendingImage = nullptr;
        }
        Wait -= Delta;
        return;
    }
    if (Game->Stage == ESAStage::Menu)
    {
        CheckAnomaliesAndStart();
        return;
    }
    if (Game->Stage == ESAStage::Ending)
    {
        VerifyEnding();
        return;
    }
    if (Game->Stage == ESAStage::HuntResult)
    {
        Game->Tick(3);
        Capture(TEXT("03-bed"));
        return;
    }
    if (Game->Stage == ESAStage::Sleep)
    {
        TrySleep();
        return;
    }
    if (Game->Stage != ESAStage::Hunt)
    {
        Finish(false, TEXT("Unexpected phase during the integration test"));
        return;
    }
    if (Scenario != Game->Cycle)
        BeginTraversal();
    if (!bComplete)
        FollowRoute(Delta);
}
