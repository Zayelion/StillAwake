#include "SAHouse.h"

#include "SACharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSAHouseDoorTest, "StillAwake.Content.DoorMotion",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSAHouseDoorTest::RunTest(const FString &Parameters)
{
    auto *World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Door motion test world created"), World))
        return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    auto *House = World->SpawnActor<ASAHouse>();
    House->Build();
    const FString DoorId = TEXT("front.door");
    auto *Door = House->Props.FindChecked(DoorId).Get();
    const FTransform Closed = Door->GetActorTransform();
    const FVector Porch = Closed.TransformPosition({-56, 44, 88});
    const FVector Foyer = Closed.TransformPosition({56, 44, 88});
    const FVector Inward = Closed.GetUnitAxis(EAxis::X);
    const FVector Across = Closed.GetUnitAxis(EAxis::Y);

    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto *Player = World->SpawnActor<ASACharacter>(Porch, FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Door presses use a player capsule"), Player))
        return false;

    const auto ReadYaw = [Door, Closed]
    {
        return FMath::FindDeltaAngleDegrees(Closed.Rotator().Yaw, Door->GetActorRotation().Yaw);
    };
    const auto Press = [House, Player](const FVector &Position, const FVector &Travel)
    {
        Player->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
        return House->UpdateDoors(.12f, Player, Travel);
    };
    const auto SweepAperture = [World, Player, Porch, Foyer]
    {
        FCollisionQueryParams Query;
        Query.AddIgnoredActor(Player);
        FHitResult Hit;
        const FVector Clearance(0, 0, 2);
        World->SweepSingleByProfile(Hit, Porch + Clearance, Foyer + Clearance, FQuat::Identity, TEXT("Pawn"),
                                    FCollisionShape::MakeCapsule(28, 88), Query);
        return Hit.GetActor();
    };
    const auto AddBox = [World, Closed](const FVector &Position, const FVector &Extent, FName Profile)
    {
        auto *Prop = World->SpawnActor<ASAProp>(Closed.TransformPosition(Position), Closed.Rotator());
        auto *Box = NewObject<UBoxComponent>(Prop);
        Prop->AddInstanceComponent(Box);
        Box->SetupAttachment(Prop->GetRootComponent());
        Box->SetBoxExtent(Extent);
        Box->SetCollisionProfileName(Profile);
        Box->RegisterComponent();
        return Box;
    };

    TestFalse(TEXT("Standing near a closed door leaves it alone"), Press(Porch, FVector::ZeroVector));
    TestFalse(TEXT("Walking along the wall does not press the door"), Press(Porch, Across));
    TestFalse(TEXT("Walking away does not press the door"), Press(Porch, -Inward));
    TestFalse(TEXT("Walking toward a door on another floor has no effect"), Press(Porch + FVector(0, 0, 342), Inward));
    TestFalse(TEXT("Walking toward the wall outside the opening has no effect"), Press(Porch + Across * 118, Inward));

    auto *Furniture = AddBox({-22, 44, 98}, {4, 23, 70}, TEXT("BlockAll"));
    TestFalse(TEXT("Furniture between the player and the door prevents a push"), Press(Porch, Inward));
    TestTrue(TEXT("Rejected presses keep the door closed"), FMath::IsNearlyZero(ReadYaw(), .1));
    Furniture->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    TestTrue(TEXT("A closed door physically blocks the player's path"), SweepAperture() == Door);
    TestTrue(TEXT("Walking into range starts opening without interact input"), Press(Porch, Inward));
    const double Started = ReadYaw();
    TestTrue(TEXT("Opening away from the porch starts at a shallow angle"), Started <= 0 && Started > -20);
    TestFalse(TEXT("The same press does not restart a moving door"), House->UpdateDoors(.55f, Player, Inward));
    const double Partial = ReadYaw();
    TestTrue(TEXT("The door passes smoothly through a partial angle"), Partial < Started && Partial > -80);

    // Put the capsule in the remaining swing to verify that the pusher is ignored by the door query.
    Player->SetActorLocation(Closed.TransformPosition({63, 26, 88}), false, nullptr, ETeleportType::TeleportPhysics);
    House->UpdateDoors(2.f, Player);
    TestTrue(TEXT("The door reaches 90 degrees without treating its pusher as furniture"),
             FMath::IsNearlyEqual(ReadYaw(), -90., 1.));
    TestNull(TEXT("An open door leaves the capsule's path clear"), SweepAperture());

    House->Reset();
    TestTrue(TEXT("Walking into range on the other side starts an opening"), Press(Foyer, -Inward));
    House->UpdateDoors(2.f, Player);
    TestTrue(TEXT("The same door opens away from the foyer"), FMath::IsNearlyEqual(ReadYaw(), 90., 1.));

    House->Reset();
    Player->SetActorLocation(Porch, false, nullptr, ETeleportType::TeleportPhysics);
    auto *Obstruction = AddBox({45, 52, 90}, {3.5, 3.5, 63}, TEXT("BlockAll"));
    House->ToggleDoor(DoorId, Porch);
    House->UpdateDoors(10.f, Player);
    const double Blocked = ReadYaw();
    TestTrue(TEXT("A single large update still stops at solid furniture in the arc"), Blocked < -20 && Blocked > -60);
    House->UpdateDoors(10.f, Player);
    TestTrue(TEXT("The leaf stays at the clear angle while the obstruction remains"),
             FMath::IsNearlyEqual(ReadYaw(), Blocked, .25));

    House->ToggleDoor(DoorId, Porch);
    House->UpdateDoors(.5f, Player);
    TestTrue(TEXT("Interact reverses the partially open leaf away from the obstruction"), ReadYaw() > Blocked);
    House->UpdateDoors(2.f, Player);
    TestTrue(TEXT("Reversing lets the door close"), FMath::IsNearlyZero(ReadYaw(), .1));

    House->ToggleDoor(DoorId, Porch);
    House->UpdateDoors(2.f, Player);
    TestTrue(TEXT("A fresh push from the far side can free a stopped door"),
             Press(Closed.TransformPosition({56, 72, 88}), -Inward));
    House->UpdateDoors(3.f, Player);
    TestTrue(TEXT("The door swings away from the new pusher with the obstacle still present"),
             FMath::IsNearlyEqual(ReadYaw(), 90., 1.));
    House->ToggleDoor(DoorId, Foyer);
    House->UpdateDoors(2.f, Player);

    Obstruction->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    House->ToggleDoor(DoorId, Porch);
    House->UpdateDoors(2.f, Player);
    TestTrue(TEXT("Retry opens fully with the same prop made decorative"), FMath::IsNearlyEqual(ReadYaw(), -90., 1.));

    House->ToggleDoor(DoorId, Porch);
    House->UpdateDoors(.5f, Player);
    TestTrue(TEXT("The door also closes gradually"), ReadYaw() > -90 && ReadYaw() < 0);
    House->Reset();
    House->UpdateDoors(2.f);
    TestTrue(TEXT("Reset clears the swing and restores the original pose"), Door->GetActorTransform().Equals(Closed));

    for (const auto &Entry : House->Props)
    {
        if (!Entry.Key.EndsWith(TEXT(".door")))
            continue;
        const FTransform Shut = Entry.Value->GetActorTransform();
        double WidestAngle = 0;
        for (double Side : {-1., 1.})
        {
            const FVector Pusher = Shut.TransformPosition({Side * 65, 45, 88});
            House->ToggleDoor(Entry.Key, Pusher);
            House->UpdateDoors(2.f, Player);
            const double Angle = FMath::Abs(FMath::FindDeltaAngleDegrees(Shut.Rotator().Yaw,
                                                                         Entry.Value->GetActorRotation().Yaw));
            WidestAngle = FMath::Max(WidestAngle, Angle);
            House->ToggleDoor(Entry.Key, Pusher);
            House->UpdateDoors(2.f, Player);
            TestTrue(Entry.Key + TEXT(" swings back to its closed pose"), Entry.Value->GetActorTransform().Equals(Shut, .01));
        }
        TestTrue(FString::Printf(TEXT("%s provides a passage (%.1f degrees)"), *Entry.Key, WidestAngle), WidestAngle > 60.);
    }
    return !HasAnyErrors();
}
#endif
