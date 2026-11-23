#include "SAHouse.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

void ASAHouse::BuildGroundFloor()
{
    auto AddFloor = [&](const FVector &Center, const FVector &Size, const FString &Material)
    {
        auto *Floor = Group(FString::Printf(TEXT("floor.%d"), Groups.Num()), Center);
        Box(Floor, {0, 0, -6}, {Size.X, Size.Y, 12}, Material);
        return Floor;
    };
    auto LayWood = [&](const FVector &Center, const FVector &Size)
    {
        auto *Floor = AddFloor(Center, Size, TEXT("walnut"));
        auto *Planks = NewObject<UInstancedStaticMeshComponent>(Floor);
        Floor->AddInstanceComponent(Planks);
        Planks->SetupAttachment(Floor->GetRootComponent());
        Planks->SetStaticMesh(Cube);
        Planks->SetMaterial(0, Materials.FindChecked(TEXT("oak")));
        Planks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Planks->RegisterComponent();
        const int Rows = FMath::CeilToInt(Size.Y / 19);
        const double RowWidth = Size.Y / Rows;
        for (int Row = 0; Row < Rows; ++Row)
        {
            const double Y = -Size.Y / 2 + (Row + .5) * RowWidth;
            double Start = -Size.X / 2;
            for (double End = Start + 127 - (Row % 3) * 38; Start < Size.X / 2; End += 127)
            {
                const double Stop = FMath::Min(End, Size.X / 2);
                Planks->AddInstance(FTransform(FQuat::Identity, {(Start + Stop) / 2, Y, .38},
                                               {FMath::Max(.1, Stop - Start - .5) / 100, (RowWidth - .45) / 100, .007}),
                                   false);
                Start = Stop;
            }
        }
    };
    AddFloor({-110, 0, 0}, {220, 520, 0}, TEXT("grout"));
    LayWood({216, 0, 0}, {432, 940, 0});
    TileFloor({777, -305, 0}, {690, 330, 0});
    LayWood({777, 165, 0}, {690, 610, 0});
    LayWood({1338, -252.5, 0}, {432, 435, 0});
    AddFloor({1338, 217.5, 0}, {432, 505, 0}, TEXT("walnut"));
    TileFloor({1707, -332.5, 0}, {306, 595, 0});
    AddFloor({2185, -415, 0}, {650, 760, 0}, TEXT("grout"));

    auto *Shell = Group(TEXT("house.shell"));
    auto WallX = [&](float X, float Y0, float Y1, float Z0 = 0, float Z1 = 342)
    {
        Box(Shell, {X, (Y0 + Y1) / 2, (Z0 + Z1) / 2}, {18, Y1 - Y0, Z1 - Z0}, TEXT("cream"));
        if (Z0 == 0)
            for (float Side : {-1.f, 1.f})
                Box(Shell, {X + Side * 9.8, (Y0 + Y1) / 2, 6}, {2, Y1 - Y0, 12}, TEXT("white"), false);
    };
    auto WallY = [&](float Y, float X0, float X1, float Z0 = 0, float Z1 = 342)
    {
        Box(Shell, {(X0 + X1) / 2, Y, (Z0 + Z1) / 2}, {X1 - X0, 18, Z1 - Z0}, TEXT("cream"));
        if (Z0 == 0)
            for (float Side : {-1.f, 1.f})
                Box(Shell, {(X0 + X1) / 2, Y + Side * 9.8, 6}, {X1 - X0, 2, 12}, TEXT("white"), false);
    };
    auto GlazeX = [&](float X, float Y, float Width)
    {
        WallX(X, Y - Width / 2, Y + Width / 2, 0, 90);
        WallX(X, Y - Width / 2, Y + Width / 2, 222, 342);
        Box(Shell, {X, Y, 156}, {5, Width, 132}, TEXT("glass"));
        for (float Z : {90.f, 156.f, 222.f})
            Box(Shell, {X, Y, Z}, {27, Width + 10, 5}, TEXT("white"), false);
        for (float YOffset : {-Width / 2, 0.f, Width / 2})
            Box(Shell, {X, Y + YOffset, 156}, {25, 6, 132}, TEXT("white"), false);
    };
    auto GlazeY = [&](float X, float Y, float Width)
    {
        WallY(Y, X - Width / 2, X + Width / 2, 0, 90);
        WallY(Y, X - Width / 2, X + Width / 2, 222, 342);
        Box(Shell, {X, Y, 156}, {Width, 5, 132}, TEXT("glass"));
        for (float Z : {90.f, 156.f, 222.f})
            Box(Shell, {X, Y, Z}, {Width + 10, 27, 5}, TEXT("white"), false);
        for (float XOffset : {-Width / 2, 0.f, Width / 2})
            Box(Shell, {X + XOffset, Y, 156}, {6, 25, 132}, TEXT("white"), false);
    };

    WallX(0, -470, -381);
    GlazeX(0, -292, 178);
    WallX(0, -203, -48);
    WallX(0, -48, 48, 208, 342);
    WallX(0, 48, 203);
    GlazeX(0, 292, 178);
    WallX(0, 381, 470);
    Door(TEXT("front.door"), {0, -48, 0}, 0);

    WallY(-470, 0, 659);
    GlazeY(739, -470, 160);
    WallY(-470, 819, 1202);
    GlazeY(1330, -470, 256);
    WallY(-470, 1458, 1554);
    WallY(470, 0, 680);
    for (float X : {760.f, 1034.f})
    {
        WallY(470, X - 80, X + 80, 0, 87);
        WallY(470, X - 80, X + 80, 243, 342);
        Window(X, X < 900);
    }
    WallY(470, 840, 954);
    WallY(470, 1114, 1122);

    WallY(-112, 0, 64);
    WallY(-112, 64, 179, 218, 342);
    WallY(-112, 179, 432);
    WallY(112, 0, 235);
    WallY(112, 235, 363, 218, 342);
    WallY(112, 363, 432);
    WallX(432, -470, -429);
    WallX(432, -429, -266, 218, 342);
    WallX(432, -266, -112);
    WallX(432, 112, 470);

    // The powder room forms the kitchen-facing end wall below the stair hall.
    WallX(635, -257, -126);
    WallY(-126, 432, 635);
    WallY(-257, 432, 509);
    WallY(-257, 509, 605, 208, 342);
    WallY(-257, 605, 635);
    AddRoomDoor(TEXT("powder.door"), {509, -257, 0}, -90);

    WallX(1122, -470, -432);
    WallX(1122, -432, -140, 218, 342);
    WallX(1122, -140, 12);
    WallX(1122, 12, 108, 208, 342);
    WallX(1122, 108, 470);
    Door(TEXT("back.door"), {1122, 108, 0}, 180);
    WallY(-35, 1122, 1256);
    WallY(-35, 1256, 1352, 208, 342);
    WallY(-35, 1352, 1554);
    AddRoomDoor(TEXT("breakfast.deck.door"), {1256, -35, 0}, -90);

    WallX(1554, -630, -466);
    WallX(1554, -466, -340, 218, 342);
    WallX(1554, -340, -178);
    WallX(1554, -54, -35);
    WallY(-630, 1554, 1860);
    WallY(-35, 1554, 2510);
    // Align the laundry partition with the solid jamb at the garage entrance.
    constexpr float LaundryWall = -537;
    WallY(LaundryWall, 1554, 1630);
    WallY(LaundryWall, 1630, 1726, 208, 342);
    WallY(LaundryWall, 1726, 1860);
    AddRoomDoor(TEXT("laundry.door"), {1630, LaundryWall, 0}, -90);
    WallX(1860, -795, -527);
    WallX(1860, -527, -431, 208, 342);
    WallX(1860, -431, -35);
    AddRoomDoor(TEXT("garage.entry.door"), {1860, -527, 0}, 0);

    WallY(-795, 1860, 2510);
    WallY(-631, 1860, 2050);
    WallY(-631, 2050, 2190, 225, 342);
    WallY(-631, 2190, 2510);
    WallX(2510, -795, -620);
    WallX(2510, -620, -344, 241, 342);
    WallX(2510, -344, -316);
    WallX(2510, -316, -45, 241, 342);
    WallX(2510, -45, -35);
    for (float Y : {-482.f, -180.5f})
    {
        auto *Bay = Group(FString::Printf(TEXT("garage.bay.%d"), int(Y)), {2510, Y, 0});
        Box(Bay, {0, 0, 120.5}, {7, 271, 241}, TEXT("tile2"));
        for (float Z : {43.f, 87.f, 131.f, 175.f, 219.f})
            Box(Bay, {-4, 0, Z}, {2, 267, 3}, TEXT("steel"), false);
    }

    auto *Porch = Group(TEXT("porch.front"));
    Box(Porch, {-110, 0, 316}, {230, 530, 15}, TEXT("wood"));
    for (float Y : {-248.f, 248.f})
    {
        Box(Porch, {-202, Y, 154}, {18, 18, 308}, TEXT("white"));
        Box(Porch, {-111, Y, 88}, {200, 8, 10}, TEXT("white"));
        for (float X : {-180.f, -145.f, -110.f, -75.f, -40.f})
            Box(Porch, {X, Y, 44}, {6, 6, 88}, TEXT("white"));
    }
    Box(Porch, {-214, 0, 85}, {8, 498, 14}, TEXT("white"));
    for (int Rail = 0; Rail < 12; ++Rail)
        Box(Porch, {-214, -223.f + Rail * 41, 43}, {5, 6, 86}, TEXT("white"));
    Box(Porch, {-50, 0, .8}, {84, 138, 1.6}, TEXT("jute"), false);

    auto *Deck = Group(TEXT("porch.back"), {1122, -35, 0});
    Box(Deck, {48, 99, 327}, {106, 208, 13}, TEXT("oak"));
    for (int Row = 0; Row < 28; ++Row)
        Box(Deck, {216, (Row + .5) * 18, .8}, {432, 17.6, 1.6}, TEXT("oak"), false);
    Box(Deck, {216, 501, 88}, {432, 8, 12}, TEXT("wood"));
    for (int Rail = 0; Rail < 12; ++Rail)
        Box(Deck, {20.f + Rail * 35, 501, 44}, {7, 7, 88}, TEXT("wood"));
    Box(Deck, {428, 252, 88}, {8, 500, 12}, TEXT("wood"));
    for (int Rail = 0; Rail < 13; ++Rail)
        Box(Deck, {428, 20.f + Rail * 39, 44}, {7, 7, 88}, TEXT("wood"));
}
