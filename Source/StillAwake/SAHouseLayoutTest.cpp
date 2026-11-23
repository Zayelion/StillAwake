#include "SAHouse.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSAHouseLayoutTest, "StillAwake.Content.HouseLayout",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSAHouseLayoutTest::RunTest(const FString &Parameters)
{
    UWorld *World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("House layout test world created"), World))
        return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    auto *House = World->SpawnActor<ASAHouse>();
    House->Build();

    const auto CheckSurface = [this, World](const FVector &Position, const TCHAR *Label)
    {
        const FVector Offset(0, 0, 12);
        FHitResult Hit;
        const bool bHit = World->SweepSingleByChannel(Hit, Position + Offset, Position - Offset, FQuat::Identity,
                                                     ECC_Visibility, FCollisionShape::MakeSphere(.5f));
        TestTrue(FString::Printf(TEXT("%s at %s (collision height %.2f)"), Label, *Position.ToString(), Hit.ImpactPoint.Z),
                 bHit && FMath::IsNearlyEqual(Hit.ImpactPoint.Z, Position.Z, 2.0));
    };
    const auto CheckSpan = [this, World](const FVector &Position, const FVector &Axis, double Nominal, const TCHAR *Label)
    {
        FHitResult Near, Far;
        const bool bNear = World->LineTraceSingleByChannel(Near, Position, Position - Nominal * Axis, ECC_Visibility);
        const bool bFar = World->LineTraceSingleByChannel(Far, Position, Position + Nominal * Axis, ECC_Visibility);
        const double Span = FVector::Dist(Near.ImpactPoint, Far.ImpactPoint);
        // Nominal dimensions include the wall faces used to lay out the plan.
        TestTrue(FString::Printf(TEXT("%s measures %.1f cm against %.1f cm nominal"), Label, Span, Nominal),
                 bNear && bFar && FMath::IsNearlyEqual(Span, Nominal, 28.0));
    };

    CheckSpan({225, -265, 230}, FVector::RightVector, 358, TEXT("Dining room width"));
    CheckSpan({170, 280, 230}, FVector::RightVector, 358, TEXT("Study width"));
    CheckSpan({1435, -250, 230}, FVector::RightVector, 435, TEXT("Breakfast room width"));
    CheckSpan({2260, -330, 230}, FVector::RightVector, 596, TEXT("Garage bay width between storage and the exterior"));
    CheckSpan({1220, -390, 570}, FVector::ForwardVector, 584, TEXT("Primary bedroom depth"));

    House->ToggleDoor(TEXT("garage.entry.door"), {1810, -479, 88});
    House->UpdateDoors(2.f);
    TestTrue(TEXT("Garage entry door completes its swing before floor probes"), House->DoorOpen(TEXT("garage.entry.door")));
    for (double Offset : {-2., 0., 2.})
    {
        CheckSurface({432 + Offset, 0, 0}, TEXT("Foyer opens onto the downstairs circulation"));
        CheckSurface({1122 + Offset, -185, 0}, TEXT("Kitchen floor meets the breakfast floor"));
        CheckSurface({1554 + Offset, -402, 0}, TEXT("Breakfast floor meets the rear entry"));
        CheckSurface({1860 + Offset, -479, 0}, TEXT("Rear entry meets the garage floor"));
    }
    CheckSurface({1345, 215, 0}, TEXT("The exterior deck has a walking surface"));
    CheckSurface({1678, -573, 0}, TEXT("The laundry is supported off the rear entry"));
    CheckSurface({2200, -572, 0}, TEXT("The rear garage bay has a walking surface"));
    CheckSurface({365, -5, 342}, TEXT("Front upstairs hall has a walking surface"));
    CheckSurface({800, 0, 342}, TEXT("Gallery overlooks the family room from a supported edge"));
    CheckSurface({1280, -70, 342}, TEXT("The primary bedroom is supported"));
    CheckSurface({1378, 175, 342}, TEXT("The primary balcony has a walking surface"));
    CheckSurface({1590, -282, 342}, TEXT("Primary rear passage connects to the media wing"));
    CheckSurface({2170, -500, 342}, TEXT("The media room floor sits over the garage"));
    CheckSurface({1950, -704, 342}, TEXT("A bathroom adjoins the media wing"));

    for (const FVector &At : {FVector(700, 195, 0), FVector(935, 235, 0), FVector(1060, 355, 0)})
    {
        FHitResult Hit;
        TestFalse(FString::Printf(TEXT("Family room is open through the upper deck at %s"), *At.ToString()),
                  World->LineTraceSingleByChannel(Hit, At + FVector(0, 0, 367), At + FVector(0, 0, 315), ECC_Visibility));
        CheckSurface(At, TEXT("The family room still has a ground floor below the void"));
    }

    const auto CheckStairs = [&](const FVector &Start, const FVector &End, const TCHAR *Label)
    {
        for (double Fraction : {.2, .5, .8})
            CheckSurface(FMath::Lerp(Start, End, Fraction), Label);
    };
    CheckSurface({493, 8, 0}, TEXT("Main stair entrance has a floor"));
    CheckStairs({493, 32, 0}, {493, 323, 171}, TEXT("Main stairs climb to the half landing"));
    CheckSurface({550, 379, 171}, TEXT("Main stair runs share a half landing"));
    CheckStairs({603, 323, 171}, {603, 32, 342}, TEXT("Main stairs return toward the upper gallery"));
    CheckSurface({603, 5, 342}, TEXT("Main stairs meet the upper gallery"));
    CheckSurface({1515, -116, 0}, TEXT("Service stair entrance has a floor"));
    CheckStairs({1543, -116, 0}, {1757, -116, 171}, TEXT("Service stair climbs from the rear entry"));
    CheckSurface({1808, -116, 171}, TEXT("Service stair turns at its half landing"));
    CheckStairs({1808, -178, 171}, {1808, -446, 342}, TEXT("Service stair climbs to the media wing"));
    CheckSurface({1808, -501, 342}, TEXT("Service stair meets the rear upstairs hall"));

    House->ToggleDoor(TEXT("upstairs.master.rear.door"), {1590, -282, 432});
    House->UpdateDoors(2.f);
    TestTrue(TEXT("Primary rear door completes its swing before the gallery sweep"),
             House->DoorOpen(TEXT("upstairs.master.rear.door")));
    FHitResult RearObstruction;
    TestFalse(TEXT("Open primary rear door leaves the gallery clear for the player capsule"),
              World->SweepSingleByProfile(RearObstruction, {1590, -282, 432.15}, {1590, -500, 432.15},
                                          FQuat::Identity, TEXT("Pawn"), FCollisionShape::MakeCapsule(28, 88)));

    auto *Door = House->Props.FindChecked(TEXT("upstairs.master.door")).Get();
    const FTransform Baseline = Door->GetActorTransform();
    House->ToggleDoor(TEXT("upstairs.master.door"), {850, -63, 430});
    House->UpdateDoors(2.f);
    TestTrue(TEXT("Primary door is open before reset"), House->DoorOpen(TEXT("upstairs.master.door")));
    House->Reset();
    TestTrue(TEXT("Hunt reset returns the primary door to the new plan's authored transform"),
             Door->GetActorTransform().Equals(Baseline));
    CheckSurface({603, 5, 342}, TEXT("Main stair remains connected after reset"));
    CheckSurface({1808, -501, 342}, TEXT("Service stair remains connected after reset"));
    return !HasAnyErrors();
}
#endif
