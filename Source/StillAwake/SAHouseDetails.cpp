#include "SAHouse.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Scene.h"

void ASAHouse::AddFixture(const FString &Id, FVector Position, float Lumens, float Radius, const FLinearColor &Color)
{
    auto *Fixture = Group(Id, Position);
    Box(Fixture, {0, 0, 36}, {27, 27, 6}, TEXT("brass"), false);
    Shape(Fixture, Cylinder, {0, 0, 30}, {12, 12, 10}, TEXT("brass"), false);
    Shape(Fixture, Sphere, {0, 0, 22}, {31, 31, 13}, TEXT("lampGlass"), false)->SetCastShadow(false);

    auto *Light = NewObject<UPointLightComponent>(Fixture);
    Fixture->AddInstanceComponent(Light);
    Light->SetupAttachment(Fixture->GetRootComponent());
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetIntensityUnits(ELightUnits::Lumens);
    const float ExposureScale = .13f; // The first-person camera uses fixed exposure.
    const float Intensity = Lumens * ExposureScale;
    Light->SetIntensity(Intensity);
    Light->SetLightColor(Color);
    Light->SetAttenuationRadius(Radius);
    Light->SetSourceRadius(8);
    Light->SetSoftSourceRadius(12);
    Light->SetShadowBias(.38f);
    Light->SetShadowSlopeBias(.45f);
    Light->SetCastShadows(true);
    Light->RegisterComponent();
    Lights.Add(Light);
    FixtureLumens.Add(Intensity);
}

void ASAHouse::ApplyBrightness(float Value, bool /*Dawn*/)
{
    BrightnessScale = Value;
    for (int Index = 0; Index < Lights.Num(); ++Index)
        Lights[Index]->SetIntensity(FixtureLumens[Index] * Value);
    ApplySleepBrightness();
}

void ASAHouse::DetailFirstFloor()
{
    const FLinearColor Warm(1, .78f, .55f);
    const FLinearColor Neutral(1, .89f, .76f);
    AddFixture(TEXT("light.frontporch"), {-113, 0, 274}, 1150, 410, Warm);
    AddFixture(TEXT("light.backporch"), {1190, 133, 285}, 1050, 430, Warm);
    AddFixture(TEXT("light.foyer"), {215, 0, 282}, 1950, 525, Warm);
    AddFixture(TEXT("light.dining"), {218, -300, 282}, 1670, 455, Warm);
    AddFixture(TEXT("light.study"), {220, 298, 282}, 1480, 445, Warm);
    AddFixture(TEXT("light.powder"), {534, -185, 282}, 750, 245, Neutral);
    AddFixture(TEXT("light.living"), {946, 50, 282}, 2100, 540, Warm);
    AddFixture(TEXT("light.kitchen"), {875, -266, 282}, 2620, 480, Neutral);
    AddFixture(TEXT("light.breakfast"), {1335, -270, 282}, 1730, 455, Warm);
    AddFixture(TEXT("light.rearEntry"), {1640, -348, 282}, 1330, 425, Warm);
    AddFixture(TEXT("light.laundry"), {1685, -566, 282}, 1175, 310, Neutral);
    AddFixture(TEXT("light.garageNear"), {2070, -400, 282}, 2300, 560, Neutral);
    AddFixture(TEXT("light.garageFar"), {2330, -287, 282}, 2250, 545, Neutral);
    AddFixture(TEXT("light.storage"), {2190, -719, 282}, 850, 410, Warm);

    auto *Sash = Props.FindChecked(TEXT("living.window")).Get();
    Box(Sash, {7, 130, -20}, {6, 10, 20}, TEXT("brass"), false);
    Box(Sash, {12, 130, -14}, {6, 8, 13}, TEXT("brass"), false);

    auto *Runner = Group(TEXT("foyer.runner"), {219, 0, 1.35});
    Box(Runner, {0, 0, 0}, {344, 93, 1}, TEXT("rug"), false)->SetCastShadow(false);
    for (float Y : {-42.f, 42.f})
        Box(Runner, {0, Y, .6}, {333, 2.5, .2}, TEXT("jute"), false)->SetCastShadow(false);
    auto *Console = Props.FindChecked(TEXT("foyer.table")).Get();
    Box(Console, {0, 0, 66}, {166, 39, 16}, TEXT("wood"), false);
    for (float X : {-53.f, 0.f, 53.f})
    {
        Box(Console, {X, 21, 67}, {49, 3, 13}, TEXT("doorInset"), false);
        Shape(Console, Sphere, {X, 24, 67}, {7, 6, 6}, TEXT("brass"), false);
    }
    Shape(Console, Cylinder, {-22, -2, 82}, {31, 27, 4}, TEXT("brass"), false);
    Box(Console, {-26, -4, 85}, {12, 3, 1.6}, TEXT("steel"), false)->SetRelativeRotation({0, 33, 0});

    auto *Sofa = Props.FindChecked(TEXT("living.sofa")).Get();
    for (float X : {-90.f, 0.f, 90.f})
    {
        Box(Sofa, {X, 28, 86}, {86, 16, 51}, TEXT("leather"), false)->SetRelativeRotation({0, 0, 9});
        Box(Sofa, {X, -45, 60}, {83, 2, 2}, TEXT("doorInset"), false);
    }
    for (float X : {-140.f, 140.f})
    {
        Shape(Sofa, Sphere, {X, -20, 91}, {25, 64, 13}, TEXT("leather"), false);
        Shape(Sofa, Sphere, {X * .79, 12, 88}, {44, 28, 42}, TEXT("canvas"), false)
            ->SetRelativeRotation({0, X > 0 ? 19.f : -19.f, -17});
    }
    auto *Rug = Group(TEXT("living.rug"), {948, 219, 1.35f});
    Box(Rug, {0, 0, 0}, {245, 285, 1}, TEXT("rug"), false)->SetCastShadow(false);
    for (float Y : {-133.f, 133.f})
        Box(Rug, {0, Y, .6}, {229, 5, .2}, TEXT("jute"), false)->SetCastShadow(false);
    for (float X : {-113.f, 113.f})
        Box(Rug, {X, 0, .6}, {5, 268, .2}, TEXT("jute"), false)->SetCastShadow(false);
    auto *TV = Props.FindChecked(TEXT("living.tv")).Get();
    Box(TV, {7.8, 0, -48}, {4, 168, 7}, TEXT("steel"), false);
    Shape(TV, Sphere, {10, 76, -48}, {2, 2, 2}, TEXT("red"), false);

    auto *Island = Props.FindChecked(TEXT("kitchen.island")).Get();
    for (float X : {-81.f, 81.f})
    {
        for (float Y : {-48.f, 48.f})
        {
            Box(Island, {X, Y, 49}, {3, 84, 65}, TEXT("doorInset"), false);
            Box(Island, {X * 1.04, Y, 68}, {5, 32, 4}, TEXT("brass"), false);
        }
        Box(Island, {X, 0, 8}, {3, 180, 12}, TEXT("black"), false);
    }
    Shape(Island, Cylinder, {-17, -29, 81.75}, {11, 11, .6}, TEXT("black"), false);
    Shape(Island, Cylinder, {-17, -29, 82.1}, {7, 7, .5}, TEXT("stainless"), false);
    Box(Island, {60, -65, 100}, {27, 38, 4}, TEXT("wood"), false)->SetRelativeRotation({0, 12, 0});
    Shape(Island, Sphere, {62, -67, 107}, {9, 9, 13}, TEXT("glass"), false);
    Shape(Island, Cylinder, {62, -67, 115}, {4, 4, 4}, TEXT("steel"), false);

    auto *FridgeDoor = Props.FindChecked(TEXT("kitchen.fridgeDoor")).Get();
    Box(FridgeDoor, {4.25, -53, 156}, {1.2, 104, 122}, TEXT("stainless"), false);
    Box(FridgeDoor, {4.25, -53, 47}, {1.2, 104, 89}, TEXT("stainless"), false);
    Box(FridgeDoor, {8, -92, 69}, {7, 5, 27}, TEXT("steel"), false);

    auto *Counter = Group(TEXT("kitchen.wallCounter"), {746, -431, 0}, -90);
    Counter->SetActorRelativeScale3D({.82, 1, 1});
    Box(Counter, {0, 0, 43}, {75, 221, 86}, TEXT("wood"));
    Box(Counter, {-2, 0, 90}, {82, 227, 8}, TEXT("tile2"));
    for (float Y : {-72.f, 0.f, 72.f})
    {
        Box(Counter, {-39, Y, 49}, {4, 67, 69}, TEXT("doorInset"), false);
        Box(Counter, {-44, Y, 71}, {7, 30, 4}, TEXT("brass"), false);
    }
    for (float Offset : {-90.f, -60.f, -30.f, 0.f, 30.f, 60.f, 90.f})
        for (float Z : {103.f, 124.f})
            Box(Counter, {34, Offset, Z}, {4, 29, 20}, TEXT("tile1"), false);
    Box(Counter, {0, 62, 112}, {53, 72, 35}, TEXT("white"), false);
    Box(Counter, {-27, 56, 112}, {4, 54, 26}, TEXT("glass"), false);
    Box(Counter, {-30, 34, 113}, {4, 4, 20}, TEXT("steel"), false);
    for (float Z : {106.f, 116.f})
        Shape(Counter, Cylinder, {-28, 90, Z}, {8, 8, 4}, TEXT("steel"), false)
            ->SetRelativeRotation({90, 0, 0});
    for (float Y : {-75.f, -53.f})
    {
        Shape(Counter, Cylinder, {6, Y, 105}, {18, 18, 20}, TEXT("tile2"), false);
        Shape(Counter, Cylinder, {6, Y, 116}, {20, 20, 4}, TEXT("wood"), false);
    }

    Table(TEXT("dining.table"), {228, -314, 0}, {188, 104, 0});
    auto *Dining = Props.FindChecked(TEXT("dining.table")).Get();
    Box(Dining, {0, 0, 80}, {171, 26, 1}, TEXT("linen"), false);
    Shape(Dining, Cylinder, {0, 0, 83}, {30, 25, 5}, TEXT("porcelain"), false);
    Shape(Dining, Sphere, {0, 0, 101}, {17, 19, 32}, TEXT("sage"), false);
    for (float X : {184.f, 274.f})
        for (float Y : {-388.f, -242.f})
        {
            auto *Seat = Group(FString::Printf(TEXT("dining.chair.%d.%d"), int(X), int(Y)), {X, Y, 0});
            Box(Seat, {0, 0, 45}, {44, 42, 7}, TEXT("oak"));
            Box(Seat, {0, Y < -314 ? -19.f : 19.f, 68}, {42, 5, 56}, TEXT("walnut"));
            for (float LegX : {-17.f, 17.f})
                for (float LegY : {-16.f, 16.f})
                    Box(Seat, {LegX, LegY, 21}, {5, 5, 42}, TEXT("oak"));
        }
    auto *Sideboard = Group(TEXT("dining.sideboard"), {290, -438, 0});
    Box(Sideboard, {0, 0, 45}, {171, 42, 90}, TEXT("walnut"));
    Box(Sideboard, {0, 0, 92}, {179, 48, 5}, TEXT("oak"));
    for (float X : {-54.f, 0.f, 54.f})
    {
        Box(Sideboard, {X, 22, 48}, {50, 3, 62}, TEXT("doorInset"), false);
        Shape(Sideboard, Sphere, {X, 24, 58}, {5, 5, 5}, TEXT("brass"), false);
    }

    Table(TEXT("study.desk"), {289, 416, 0}, {150, 64, 0});
    auto *Desk = Props.FindChecked(TEXT("study.desk")).Get();
    Box(Desk, {-49, 0, 51}, {48, 58, 42}, TEXT("walnut"));
    Box(Desk, {12, -8, 81}, {38, 28, 3}, TEXT("white"), false);
    Box(Desk, {26, -17, 84}, {14, 20, 2}, TEXT("slate"), false)->SetRelativeRotation({0, 18, 0});
    Shape(Desk, Cylinder, {50, 15, 82}, {17, 17, 5}, TEXT("brass"), false);
    Shape(Desk, Cylinder, {50, 15, 100}, {4, 4, 34}, TEXT("brass"), false);
    Shape(Desk, Cone, {50, 15, 118}, {25, 25, 19}, TEXT("lampGlass"), false)->SetCastShadow(false);
    auto *StudyChair = Group(TEXT("study.chair"), {132, 311, 0});
    Box(StudyChair, {0, 0, 25}, {96, 88, 44}, TEXT("slate"));
    Box(StudyChair, {-36, 0, 60}, {18, 88, 92}, TEXT("slate"));
    for (float Y : {-39.f, 39.f})
        Box(StudyChair, {0, Y, 48}, {92, 14, 35}, TEXT("slate"));
    auto *Books = Group(TEXT("study.books"), {397, 309, 0});
    Box(Books, {14, 0, 85}, {4, 132, 170}, TEXT("walnut"));
    for (float Y : {-63.f, 63.f})
        Box(Books, {0, Y, 85}, {32, 4, 170}, TEXT("oak"));
    for (int Shelf = 0; Shelf < 4; ++Shelf)
    {
        Box(Books, {0, 0, 12.f + 39 * Shelf}, {34, 131, 4}, TEXT("oak"));
        for (int Book = 0; Book < 7; ++Book)
            Box(Books, {1, -54.f + 16 * Book, 25.f + 39 * Shelf}, {19, 10, 18.f + Book % 3 * 4},
                Book % 2 == 0 ? TEXT("sage") : TEXT("slate"), false);
    }

    auto *Hearth = Group(TEXT("living.hearth"), {894, 447, 0});
    Box(Hearth, {0, 2, 48}, {106, 32, 96}, TEXT("doorInset"));
    Box(Hearth, {0, -15, 38}, {65, 3, 58}, TEXT("black"), false);
    for (float X : {-26.f, -13.f, 0.f, 13.f, 26.f})
        Box(Hearth, {X, -18, 38}, {2, 2, 50}, TEXT("steel"), false);
    Box(Hearth, {0, -4, 92}, {127, 47, 8}, TEXT("oak"), false);
    Box(Hearth, {0, -23, 3}, {124, 49, 6}, TEXT("tile2"), false);
    Box(Hearth, {0, 10, 162}, {87, 14, 134}, TEXT("cream"), false);
    auto *Media = Group(TEXT("living.mediaConsole"), {1060, 235, 0}, 180);
    Box(Media, {0, 0, 23}, {72, 161, 44}, TEXT("walnut"));
    Box(Media, {35, 0, 23}, {3, 153, 38}, TEXT("doorInset"), false);
    for (float Y : {-49.f, 0.f, 49.f})
        Box(Media, {38, Y, 28}, {3, 18, 3}, TEXT("brass"), false);

    auto *Powder = Group(TEXT("powder.fixtures"), {446, -193, 0});
    Box(Powder, {19, -31, 40}, {38, 39, 80}, TEXT("walnut"));
    Shape(Powder, Sphere, {20, -31, 81}, {40, 41, 9}, TEXT("porcelain"));
    Shape(Powder, Cylinder, {7, -32, 89}, {4, 4, 16}, TEXT("steel"), false);
    Box(Powder, {1, -31, 141}, {4, 34, 66}, TEXT("mirror"), false);
    Shape(Powder, Sphere, {28, 29, 39}, {48, 35, 25}, TEXT("porcelain"));
    Box(Powder, {8, 28, 58}, {14, 38, 38}, TEXT("porcelain"));
    Shape(Powder, Cylinder, {24, 29, 18}, {32, 26, 36}, TEXT("porcelain"));

    auto *Breakfast = Props.FindChecked(TEXT("kitchen.table")).Get();
    Box(Breakfast, {0, 0, 80}, {111, 25, 1}, TEXT("linen"), false);
    Shape(Breakfast, Cylinder, {17, 0, 83}, {38, 26, 6}, TEXT("porcelain"), false);
    Shape(Breakfast, Sphere, {17, 0, 88}, {22, 16, 14}, TEXT("sage"), false);
    Table(TEXT("deck.table"), {1332, 324, 0}, {111, 83, 0});
    auto *DeckSeat = Group(TEXT("deck.bench"), {1425, 446, 0});
    Box(DeckSeat, {0, 0, 42}, {115, 40, 6}, TEXT("oak"));
    for (float X : {-49.f, 49.f})
        Box(DeckSeat, {X, 0, 21}, {8, 38, 42}, TEXT("wood"));
    Box(DeckSeat, {0, 17, 65}, {115, 7, 42}, TEXT("oak"));

    for (float X : {1598.f, 1810.f})
    {
        auto *Appliance = Group(FString::Printf(TEXT("laundry.appliance.%d"), int(X)), {X, -585, 0});
        Box(Appliance, {0, 0, 45}, {60, 66, 90}, TEXT("porcelain"));
        Shape(Appliance, Cylinder, {0, 34, 43}, {37, 37, 5}, TEXT("steel"), false)
            ->SetRelativeRotation({0, 0, 90});
        Shape(Appliance, Cylinder, {0, 37, 43}, {28, 28, 3}, TEXT("glass"), false)
            ->SetRelativeRotation({0, 0, 90});
        Box(Appliance, {-12, 34, 78}, {21, 4, 8}, TEXT("doorInset"), false);
        Shape(Appliance, Sphere, {17, 36, 79}, {7, 6, 7}, TEXT("steel"), false);
    }
    auto *Mudroom = Group(TEXT("rearEntry.hooks"), {1595, -349, 0});
    Box(Mudroom, {0, 0, 152}, {65, 4, 13}, TEXT("oak"), false);
    for (float X : {-22.f, 0.f, 22.f})
        Box(Mudroom, {X, -4, 148}, {3, 8, 8}, TEXT("brass"), false);

    auto *Vehicle = Group(TEXT("garage.car"), {2230, -418, 0});
    Shape(Vehicle, Sphere, {0, 0, 57}, {366, 170, 75}, TEXT("slate"));
    Shape(Vehicle, Sphere, {-26, 0, 103}, {176, 134, 65}, TEXT("glass"));
    for (float X : {-112.f, 112.f})
        for (float Y : {-74.f, 74.f})
        {
            Shape(Vehicle, Cylinder, {X, Y, 30}, {59, 59, 23}, TEXT("black"))
                ->SetRelativeRotation({0, 0, 90});
            Shape(Vehicle, Cylinder, {X, Y + FMath::Sign(Y) * 12, 30}, {32, 32, 2}, TEXT("steel"), false)
                ->SetRelativeRotation({0, 0, 90});
        }
    for (float Y : {-51.f, 51.f})
        Box(Vehicle, {161, Y, 65}, {13, 27, 13}, TEXT("lampGlass"), false);
    for (float Y : {-67.f, 67.f})
        Box(Vehicle, {-161, Y, 62}, {13, 19, 12}, TEXT("red"), false);

    auto *Rack = Group(TEXT("garage.storageShelves"), {2230, -752, 0});
    for (float X : {-130.f, 0.f, 130.f})
        Box(Rack, {X, 0, 87}, {5, 46, 174}, TEXT("steel"));
    for (float Z : {16.f, 67.f, 116.f, 171.f})
        Box(Rack, {0, 0, Z}, {266, 48, 4}, TEXT("wood"));
    for (float X : {-96.f, -44.f, 16.f, 82.f})
        Box(Rack, {X, 0, 83}, {37, 31, 28}, TEXT("canvas"));
    for (float X : {-75.f, 15.f, 84.f})
        Box(Rack, {X, 0, 135}, {42, 35, 32}, TEXT("doorInset"));
}
