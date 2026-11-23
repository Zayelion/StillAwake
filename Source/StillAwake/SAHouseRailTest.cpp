#include "SAHouse.h"

#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSAHouseRailTest, "StillAwake.Content.StairRails",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSAHouseRailTest::RunTest(const FString &Parameters)
{
    UWorld *World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Stair rail test world created"), World))
        return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    auto *House = World->SpawnActor<ASAHouse>();
    House->Build();

    const auto *Guards = House->Props.FindChecked(TEXT("upstairs.guards")).Get();
    FCollisionQueryParams RailsOnly;
    for (const auto &Group : House->Groups)
        if (Group.Get() != Guards)
            RailsOnly.AddIgnoredActor(Group.Get());

    const auto CheckGuard = [&](const FVector &Start, const FVector &End, const TCHAR *Label)
    {
        const FVector Side = FVector::CrossProduct(FVector::UpVector, (End - Start).GetSafeNormal2D()) * 17;
        const int Samples = FMath::CeilToInt(FVector::Dist2D(Start, End) / 2);
        for (int I = 0; I <= Samples; ++I)
        {
            const FVector Foot = FMath::Lerp(Start, End, double(I) / Samples);
            const FVector Lower = Foot + FVector(0, 0, 35);
            const FVector Top = Foot + FVector(0, 0, 91);
            FHitResult Hit;
            if (!World->SweepSingleByChannel(Hit, Lower - Side, Lower + Side, FQuat::Identity, ECC_Pawn,
                                             FCollisionShape::MakeSphere(4.5), RailsOnly) ||
                !World->LineTraceTestByChannel(Top - Side, Top + Side, ECC_Pawn, RailsOnly))
            {
                AddError(FString::Printf(TEXT("%s has a gap at %s"), Label, *Foot.ToString()));
                return;
            }
        }
    };
    CheckGuard({443, 32, 0}, {443, 323, 171}, TEXT("Main stair front outer rail"));
    CheckGuard({537, 32, 0}, {537, 323, 171}, TEXT("Main stair front inner rail"));
    CheckGuard({556, 323, 171}, {556, 32, 342}, TEXT("Main stair return inner rail"));
    CheckGuard({652, 323, 171}, {652, 32, 342}, TEXT("Main stair return outer rail"));
    CheckGuard({537, 323, 171}, {556, 323, 171}, TEXT("Main stair half landing notch"));
    CheckGuard({652, 323, 171}, {652, 430, 171}, TEXT("Main stair half landing family side"));
    CheckGuard({443, 430, 171}, {652, 430, 171}, TEXT("Main stair half landing end"));
    CheckGuard({660, 32, 342}, {660, 110, 342}, TEXT("Main stair gallery shaft edge"));
    CheckGuard({660, 110, 342}, {930, 110, 342}, TEXT("Family room gallery edge"));
    CheckGuard({660, 430, 342}, {660, 470, 342}, TEXT("Upper stair enclosure edge"));
    CheckGuard({1543, -164, 0}, {1757, -164, 171}, TEXT("Service stair first inner rail"));
    CheckGuard({1543, -68, 0}, {1757, -68, 171}, TEXT("Service stair first outer rail"));
    CheckGuard({1757, -164, 171}, {1761, -178, 171}, TEXT("Service stair half landing inner jog"));
    CheckGuard({1757, -68, 171}, {1855, -68, 171}, TEXT("Service stair half landing outer edge"));
    CheckGuard({1761, -178, 171}, {1761, -446, 342}, TEXT("Service stair return inner rail"));
    CheckGuard({1855, -178, 171}, {1855, -446, 342}, TEXT("Service stair return outer rail"));
    CheckGuard({1654, -360, 342}, {1654, -54, 342}, TEXT("Rear upstairs lightwell edge"));
    CheckGuard({1654, -360, 342}, {1757, -360, 342}, TEXT("Rear upstairs lightwell turn"));
    CheckGuard({1757, -360, 342}, {1757, -446, 342}, TEXT("Rear upstairs lightwell return"));

    const auto CheckPath = [&](const TArray<FVector> &Route, const TCHAR *Label)
    {
        const FVector CapsuleLift(0, 0, 89);
        for (int I = 1; I < Route.Num(); ++I)
        {
            FHitResult Hit;
            TestFalse(FString::Printf(TEXT("%s is clear for the player at segment %d"), Label, I),
                      World->SweepSingleByChannel(Hit, Route[I - 1] + CapsuleLift, Route[I] + CapsuleLift,
                                                  FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(28, 88),
                                                  RailsOnly));
        }
    };
    CheckPath({{493, 8, 0}, {493, 32, 0}, {493, 323, 171}, {493, 379, 171}, {603, 379, 171},
               {603, 323, 171}, {603, 32, 342}, {603, -15, 342}, {800, 0, 342}},
              TEXT("Main stair and gallery route"));
    CheckPath({{1515, -116, 0}, {1543, -116, 0}, {1757, -116, 171}, {1808, -116, 171}, {1808, -178, 171},
               {1808, -446, 342}, {1808, -501, 342}},
              TEXT("Service stair and rear landing route"));
    return !HasAnyErrors();
}
#endif
