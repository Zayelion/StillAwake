#include "SAShadowEyes.h"

#include "SAHouse.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSAShadowEyesTest, "StillAwake.Horror.ShadowEyes",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSAShadowEyesTest::RunTest(const FString &Parameters)
{
    UWorld *World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world created"), World))
        return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    auto *House = World->SpawnActor<ASAHouse>();
    House->Build();
    auto *Eyes = World->SpawnActor<ASAShadowEyes>();
    auto *FoyerLamp = House->Props.FindChecked(TEXT("light.foyer"))->FindComponentByClass<UPointLightComponent>();
    for (const auto &Light : House->Lights)
        Light->SetVisibility(Light.Get() == FoyerLamp);

    const FVector View(75, 0, 155);
    const FVector Aim = FVector::RightVector;
    Eyes->Observe(House, View, Aim, nullptr, 11.f);
    TestTrue(TEXT("Looking at a lit wall leaves the eyes hidden"), Eyes->IsHidden());

    const FVector WallPoint(75, 103, 155);
    const FVector ShadePosition = (WallPoint + FoyerLamp->GetComponentLocation()) / 2.;
    auto *Shade = World->SpawnActor<ASAProp>(ShadePosition, FRotator::ZeroRotator);
    auto *Occluder = NewObject<UBoxComponent>(Shade);
    Shade->AddInstanceComponent(Occluder);
    Occluder->SetupAttachment(Shade->GetRootComponent());
    Occluder->SetBoxExtent(FVector(23));
    Occluder->SetCollisionProfileName(TEXT("BlockAll"));
    Occluder->RegisterComponent();

    Eyes->Observe(House, View, Aim, nullptr, 10.f);
    TestTrue(TEXT("Exactly ten seconds of staring is too early"), Eyes->IsHidden());
    Eyes->Observe(House, View, Aim, nullptr, .35f);
    TestFalse(TEXT("A wall in shadow reveals eyes even with the light on"), Eyes->IsHidden());
    TestTrue(TEXT("The eyes are anchored just in front of the wall"),
             Eyes->GetActorLocation().Equals(WallPoint - Aim * 1.4, .2));
    TestTrue(TEXT("Eye meshes stay at their authored size"), Eyes->GetActorScale3D().Equals(FVector::OneVector));

    TArray<UStaticMeshComponent *> Parts;
    Eyes->GetComponents(Parts);
    if (!TestEqual(TEXT("Two eyes have a white, iris and pupil"), Parts.Num(), 6))
        return false;
    for (const auto *Part : Parts)
        TestTrue(TEXT("Eyes do not obstruct gameplay"), Part->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
    const double OpenHeight = Parts[0]->GetRelativeScale3D().Z;

    Eyes->Observe(House, View, Aim, nullptr, 2.25f);
    TestTrue(TEXT("The eyes nearly close for a blink"), Parts[0]->GetRelativeScale3D().Z < OpenHeight * .2);
    Eyes->Observe(House, View, Aim, nullptr, .2f);
    TestTrue(TEXT("The eyes reopen after the blink"), Parts[0]->GetRelativeScale3D().Z > OpenHeight * .9);

    Eyes->Observe(House, View, FVector(.38, 1, 0).GetSafeNormal(), nullptr, .1f);
    TestTrue(TEXT("Looking to another spot hides the eyes"), Eyes->IsHidden());
    Eyes->Observe(House, View, Aim, nullptr, 9.9f);
    TestTrue(TEXT("Returning gaze requires a new uninterrupted stare"), Eyes->IsHidden());
    Eyes->Observe(House, View, Aim, nullptr, .2f);
    TestFalse(TEXT("The original shadow can reveal the eyes again"), Eyes->IsHidden());
    Occluder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Eyes->Observe(House, View, Aim, nullptr, .15f);
    TestTrue(TEXT("Lighting the wall hides the eyes"), Eyes->IsHidden());

    FoyerLamp->SetVisibility(false);
    Eyes->Observe(House, View, Aim, nullptr, 6.f);
    Eyes->Reset();
    Eyes->Observe(House, View, Aim, nullptr, 6.f);
    TestTrue(TEXT("Reset discards a partial stare"), Eyes->IsHidden());
    Eyes->Observe(House, {1320, 155, 155}, -FVector::RightVector, nullptr, 11.f);
    TestTrue(TEXT("Watching the breakfast room from the outdoor deck keeps the eyes hidden"), Eyes->IsHidden());
    return !HasAnyErrors();
}
#endif
