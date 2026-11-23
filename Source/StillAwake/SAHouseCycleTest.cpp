#include "SAHouse.h"
#include "SAGameMode.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSAHouseCycleTest, "StillAwake.Content.HuntAndSleepScene",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSAHouseCycleTest::RunTest(const FString &Parameters)
{
    UWorld *World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world created"), World))
        return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    auto *House = World->SpawnActor<ASAHouse>();
    House->Build();

    auto *Game = World->SpawnActor<ASAGameMode>();
    Game->House = House;
    FString LoadError;
    if (!TestTrue(TEXT("Load the house's authored anomalies"), Game->Content.Load(LoadError)))
        return false;
    const FString DrawingId = TEXT("upstairs.child.drawing");
    const int32 DrawingIndex = Game->Content.Anomalies.IndexOfByPredicate(
        [&](const FSAAnomaly &Anomaly) { return Anomaly.Target == DrawingId; });
    if (!TestTrue(TEXT("The drawing has a relocation anomaly"), DrawingIndex != INDEX_NONE))
        return false;
    auto *Drawing = House->Props.FindChecked(DrawingId).Get();
    const FVector ChildWall = Drawing->GetActorLocation();
    Game->ApplyAnomaly(DrawingIndex);
    const FVector KitchenWall = Drawing->GetActorLocation();
    TestTrue(TEXT("The missing drawing reappears on the kitchen face of the powder room wall"),
             KitchenWall.Equals(FVector(648, -195, 157), .1));
    const FVector KitchenView(765, -170, 162);
    TestEqual(TEXT("The kitchen drawing can be marked from the aisle"),
              House->FindInspectable(KitchenView, KitchenWall - KitchenView, {DrawingId}, nullptr), DrawingId);
    House->Reset();
    TestTrue(TEXT("A new hunt puts the drawing back in the child's bedroom"),
             Drawing->GetActorLocation().Equals(ChildWall));

    const FString Screen = TEXT("living.static");
    const TSet<FString> ScreenTargets{Screen};
    auto *TV = House->Props.FindChecked(TEXT("living.tv")).Get();
    const FVector Facing = TV->GetActorForwardVector();
    const FVector View = TV->GetActorLocation() + Facing * 225;
    const FVector Aim = -Facing;
    FVector StickerPoint;
    TestEqual(TEXT("The normal TV screen is a valid judgment target"),
              House->FindInspectable(View, Aim, ScreenTargets, nullptr, &StickerPoint), Screen);
    TestTrue(TEXT("The family room wall blocks judgment of the screen from the deck"),
             House->FindInspectable(TV->GetActorLocation() + Aim * 100, -Aim, ScreenTargets, nullptr).IsEmpty());
    const FVector DistantView = View - Aim * 140;
    FHitResult DistantHit;
    World->LineTraceSingleByChannel(DistantHit, DistantView, DistantView + Aim * 430, ECC_Visibility);
    TestTrue(TEXT("The unobstructed screen remains out of reach beyond the world-space inspection distance"),
             DistantHit.GetActor() == TV && House->FindInspectable(DistantView, Aim, ScreenTargets, nullptr).IsEmpty());

    House->AddSticker(Screen, StickerPoint, View);
    ASAProp *Sticker = nullptr;
    for (TActorIterator<ASAProp> It(World); It; ++It)
        if (It->ActorHasTag(TEXT("FixItSticker")))
        {
            Sticker = *It;
            break;
        }
    if (TestNotNull(TEXT("Marking the normal screen creates a world sticker"), Sticker))
    {
        TestTrue(TEXT("Sticker is attached to the visible TV housing"), Sticker->GetAttachParentActor() == TV);
        const FVector InitialPosition = Sticker->GetActorLocation();
        const FVector Offset(20, 0, 0);
        TV->AddActorWorldOffset(Offset);
        TestTrue(TEXT("Sticker follows its parent"), Sticker->GetActorLocation().Equals(InitialPosition + Offset));
    }
    House->Reset();
    for (TActorIterator<ASAProp> It(World); It; ++It)
        TestFalse(TEXT("Hunt reset clears the judgment sticker"), It->ActorHasTag(TEXT("FixItSticker")));

    House->Props.FindChecked(Screen)->Visible(true);
    TestEqual(TEXT("The active static uses the same judgment target"),
              House->FindInspectable(View, Aim, ScreenTargets, nullptr), Screen);

    const FString Photo = TEXT("upstairs.master.photo");
    House->Props.FindChecked(Photo)->Visible(false);
    const FVector PhotoPosition = House->Props.FindChecked(Photo)->GetActorLocation();
    House->AddSticker(Photo, PhotoPosition, PhotoPosition + FVector(-90, 180, 83));
    auto *Nightstand = House->Props.FindChecked(TEXT("upstairs.master.nightstand")).Get();
    const FVector PhotoMark(PhotoPosition.X, PhotoPosition.Y, Nightstand->GetActorLocation().Z + 68.8);
    bool bMarkedNightstand = false;
    for (TActorIterator<ASAProp> It(World); It; ++It)
        if (It->ActorHasTag(TEXT("FixItSticker")))
            bMarkedNightstand = It->GetAttachParentActor() == Nightstand && It->GetActorLocation().Equals(PhotoMark, .2);
    TestTrue(TEXT("Missing photo leaves a sticker on the nightstand"), bMarkedNightstand);

    auto *Leaf = House->Props.FindChecked(TEXT("upstairs.master.wardrobe.leaf")).Get();
    auto *Presence = House->Props.FindChecked(TEXT("upstairs.master.wardrobe.presence")).Get();
    auto *Lamp = House->Props.FindChecked(TEXT("sleep.bedsideLight"))->FindComponentByClass<UPointLightComponent>();
    auto *Ceiling = House->Props.FindChecked(TEXT("upstairs.master.light"))->FindComponentByClass<UPointLightComponent>();
    auto *Body = Presence->FindComponentByClass<UStaticMeshComponent>();
    if (!TestNotNull(TEXT("Closet presence has a body mesh"), Body))
        return false;
    auto *BodyMaterial = Body->GetMaterial(0);
    if (!TestNotNull(TEXT("Closet presence has a fade material"), BodyMaterial))
        return false;
    TestTrue(TEXT("Closet body fades with a masked material"), BodyMaterial->GetBlendMode() == BLEND_Masked);
    const auto ReadOpacity = [this, BodyMaterial]
    {
        float Opacity = -1.f;
        TestTrue(TEXT("Closet body exposes its opacity"),
                 BodyMaterial->GetScalarParameterValue(FName(TEXT("PresenceOpacity")), Opacity));
        return Opacity;
    };
    const FVector BedView = House->BedViewLocation();
    FHitResult Hit;
    World->LineTraceSingleByChannel(Hit, BedView, BedView + ASAHouse::BedViewRotation().Vector() * 430, ECC_Visibility);
    TestTrue(TEXT("Resting view can see the closet door"), Hit.GetActor() == Leaf);

    House->SetSleepScene(true, false, .75f);
    TestFalse(TEXT("The relocated bedroom ceiling light switches off for sleep"), Ceiling->IsVisible());
    const double OpenYaw = Leaf->GetActorRotation().Yaw;
    TestTrue(TEXT("Closet opens gradually in darkness"), OpenYaw > 50 && OpenYaw < 80);
    TestFalse(TEXT("Bedside lamp starts dark"), Lamp->IsVisible());
    TestTrue(TEXT("Closet body is fully hidden in the dark"), Presence->IsHidden() && ReadOpacity() == 0.f);
    const FVector DarkLocation = Presence->GetActorLocation();

    House->SetSleepScene(true, true, .75f);
    TestTrue(TEXT("Lamp turns on immediately and the body starts from nothing"),
             Lamp->IsVisible() && Presence->IsHidden() && ReadOpacity() == 0.f);
    TestTrue(TEXT("Lamp switch keeps existing closet progress"),
             FMath::IsNearlyEqual(Leaf->GetActorRotation().Yaw, OpenYaw) &&
                 Presence->GetActorLocation().Equals(DarkLocation));

    House->SetSleepScene(true, true, .65f, false, .18f);
    const float Partial = ReadOpacity();
    TestTrue(TEXT("Body gradually fades in while the door retreats"),
             Partial > 0.f && Partial < 1.f && !Presence->IsHidden() && Leaf->GetActorRotation().Yaw < OpenYaw);
    TestFalse(TEXT("Closet body does not block collisions"), Presence->GetActorEnableCollision());

    House->SetSleepScene(true, false, .65f);
    TestTrue(TEXT("Switching off hides the body and clears opacity without advancing time"),
             Presence->IsHidden() && ReadOpacity() == 0.f && !Lamp->IsVisible());
    House->SetSleepScene(true, false, .8f, false, .4f);
    TestTrue(TEXT("Closet keeps advancing while the body remains hidden"),
             Leaf->GetActorRotation().Yaw > OpenYaw && Presence->IsHidden() && ReadOpacity() == 0.f);

    House->SetSleepScene(true, true, .8f, false, .09f);
    TestTrue(TEXT("Restored lamp starts a fresh fade"), ReadOpacity() > 0.f && ReadOpacity() < Partial);
    House->SetSleepScene(true, true, .4f, false, 1.f);
    TestTrue(TEXT("Fade reaches full opacity"), ReadOpacity() == 1.f && !Presence->IsHidden());

    House->Reset();
    TestTrue(TEXT("The bedroom ceiling light comes back on for inspection"), Ceiling->IsVisible());
    TestTrue(TEXT("Hunt reset returns the bedroom to its baseline"),
             FMath::IsNearlyZero(Leaf->GetActorRotation().Yaw) && Presence->IsHidden() && !Lamp->IsVisible() &&
                 ReadOpacity() == 0.f);
    return !HasAnyErrors();
}
#endif
