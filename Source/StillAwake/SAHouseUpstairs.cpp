#include "SAHouse.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"

namespace
{
constexpr double Landing = 342;
constexpr double HalfLanding = Landing / 2;
constexpr double Ceiling = Landing + 270;
}

void ASAHouse::BuildUpstairs()
{
    auto *Structure = Group(TEXT("upstairs.structure"));
    auto *Planks = NewObject<UInstancedStaticMeshComponent>(Structure);
    Structure->AddInstanceComponent(Planks);
    Planks->SetupAttachment(Structure->GetRootComponent());
    Planks->SetStaticMesh(Cube);
    Planks->SetMaterial(0, Materials.FindChecked(TEXT("oak")).Get());
    Planks->SetCollisionProfileName(TEXT("NoCollision"));
    Planks->RegisterComponent();
    auto Deck = [&](double X0, double X1, double Y0, double Y1, bool Wood = true)
    {
        Box(Structure, {(X0 + X1) / 2, (Y0 + Y1) / 2, Landing - 11}, {X1 - X0, Y1 - Y0, 22},
            TEXT("ceiling"));
        if (!Wood)
            return;
        for (int Row = 0; Y0 + Row * 19 < Y1; ++Row)
        {
            const double Y = Y0 + Row * 19;
            const double Width = FMath::Min(19., Y1 - Y);
            for (double X = X0 - (Row % 3) * 42; X < X1; X += 126)
            {
                const double Start = FMath::Max(X0, X);
                const double End = FMath::Min(X1, X + 126);
                Planks->AddInstance(FTransform(FQuat::Identity, {(Start + End) / 2, Y + Width / 2, Landing + .15},
                                                {(End - Start - .25) / 100, (Width - .27) / 100, .003}));
            }
        }
    };
    auto Roof = [&](double X0, double X1, double Y0, double Y1)
    {
        Box(Structure, {(X0 + X1) / 2, (Y0 + Y1) / 2, Ceiling + 8}, {X1 - X0, Y1 - Y0, 16}, TEXT("ceiling"));
    };

    Deck(0, 255, -470, -141);
    Deck(0, 255, -141, 132, false);
    Deck(0, 255, 132, 470);
    Deck(255, 438, -470, 470);
    Deck(438, 660, -470, 32);
    Deck(438, 660, 430, 470);
    Deck(660, 930, -470, -160, false);
    Deck(660, 930, -160, 110);
    Deck(930, 1514, -470, 94);
    Deck(1122, 1470, 94, 232);
    Deck(1514, 1860, -630, -446);
    Deck(1514, 1757, -446, -360);
    Deck(1514, 1654, -360, -35);
    Deck(1654, 1860, -54, -35);
    Deck(1860, 2040, -795, -630, false);
    Deck(1860, 2040, -630, -35);
    Deck(2040, 2510, -795, -35);

    Roof(0, 1122, -470, 470);
    Roof(1122, 1514, -470, 94);
    Roof(1514, 1860, -630, -35);
    Roof(1860, 2510, -795, -35);

    TileFloor({127.5, -4.5, Landing + .8}, {255, 273, 0});
    TileFloor({795, -315, Landing + .8}, {270, 310, 0});
    TileFloor({1950, -712.5, Landing + .8}, {180, 165, 0});

    auto Wall = [&](bool AlongX, double Fixed, double A, double B, double Bottom = Landing, double Top = Ceiling)
    {
        const double Middle = (A + B) / 2;
        const double Height = Top - Bottom;
        const double Z = Bottom + Height / 2;
        Box(Structure, AlongX ? FVector(Middle, Fixed, Z) : FVector(Fixed, Middle, Z),
            AlongX ? FVector(B - A, 18, Height) : FVector(18, B - A, Height), TEXT("cream"));
        if (Bottom != Landing)
            return;
        for (double Offset : {-9.5, 9.5})
            Box(Structure, AlongX ? FVector(Middle, Fixed + Offset, Landing + 5) :
                                      FVector(Fixed + Offset, Middle, Landing + 5),
                AlongX ? FVector(B - A, 2, 10) : FVector(2, B - A, 10), TEXT("white"), false);
    };
    auto DoorOpening = [&](const TCHAR *Id, bool AlongX, double Fixed, double Start, double End, double Center,
                           bool FirstHinge)
    {
        const double Left = Center - DoorWidth / 2;
        const double Right = Center + DoorWidth / 2;
        Wall(AlongX, Fixed, Start, Left);
        Wall(AlongX, Fixed, Right, End);
        Wall(AlongX, Fixed, Left, Right, Landing + DoorHeight, Ceiling);
        const double Hinge = FirstHinge ? Left : Right;
        if (AlongX)
        {
            AddRoomDoor(Id, {Hinge, Fixed, Landing}, FirstHinge ? -90 : 90);
            return;
        }
        AddRoomDoor(Id, {Fixed, Hinge, Landing}, FirstHinge ? 0 : 180);
    };
    auto WindowWall = [&](const TCHAR *Id, bool AlongX, double Fixed, double A, double B, double Center, double Width)
    {
        const double Left = Center - Width / 2;
        const double Right = Center + Width / 2;
        Wall(AlongX, Fixed, A, Left);
        Wall(AlongX, Fixed, Right, B);
        Wall(AlongX, Fixed, Left, Right, Landing, Landing + 92);
        Wall(AlongX, Fixed, Left, Right, Landing + 219, Ceiling);
        auto *Frame = Group(Id, AlongX ? FVector(Center, Fixed, Landing) : FVector(Fixed, Center, Landing),
                            AlongX ? 90 : 0);
        Box(Frame, {0, 0, 155.5}, {6, Width, 127}, TEXT("glass"));
        for (double Position : {-Width / 2, 0., Width / 2})
            Box(Frame, {0, Position, 155.5}, {22, 6, 132}, TEXT("white"), false);
        for (double Height : {92., 155.5, 219.})
            Box(Frame, {0, 0, Height}, {25, Width + 12, 6}, TEXT("white"), false);
        Box(Frame, {0, 0, 91}, {33, Width + 18, 5}, TEXT("oak"), false);
    };

    WindowWall(TEXT("upstairs.child.window"), false, 0, -470, -141, -314, 158);
    WindowWall(TEXT("upstairs.bath.window"), false, 0, -141, 132, -9, 82);
    WindowWall(TEXT("upstairs.third.window"), false, 0, 132, 470, 302, 158);
    WindowWall(TEXT("upstairs.west.window"), true, -470, 0, 432, 193, 142);
    Wall(true, -470, 432, 930);
    WindowWall(TEXT("upstairs.master.window"), true, -470, 930, 1514, 1082, 189);
    WindowWall(TEXT("upstairs.east.window"), true, 470, 0, 432, 212, 138);
    Wall(true, 470, 432, 660);
    WindowWall(TEXT("upstairs.family.window"), true, 470, 660, 1122, 894, 222);
    WindowWall(TEXT("upstairs.family.rear.window"), false, 1122, 110, 470, 342, 162);

    DoorOpening(TEXT("upstairs.child.door"), true, -141, 0, 432, 363, true);
    DoorOpening(TEXT("upstairs.third.door"), true, 132, 0, 432, 363, false);
    DoorOpening(TEXT("upstairs.bath.door"), false, 255, -141, 132, -5, false);
    Wall(false, 432, -470, -141);
    Wall(false, 432, 32, 470);
    Wall(true, -260, 432, 660);
    Wall(true, -160, 660, 930);
    DoorOpening(TEXT("upstairs.walkin.door"), false, 660, -470, -260, -341, false);
    Wall(false, 660, -260, -160);

    DoorOpening(TEXT("upstairs.ensuite.door"), false, 930, -470, -160, -262, false);
    DoorOpening(TEXT("upstairs.master.door"), false, 930, -160, 110, -63, true);
    Wall(true, 94, 930, 1122);
    DoorOpening(TEXT("upstairs.balcony.door"), true, 94, 1122, 1514, 1378, false);
    DoorOpening(TEXT("upstairs.master.rear.door"), false, 1514, -470, 94, -282, false);
    Wall(false, 1514, -630, -470);

    WindowWall(TEXT("upstairs.rear.landing.window"), true, -630, 1514, 1860, 1660, 128);
    Wall(true, -35, 1514, 1860);
    WindowWall(TEXT("upstairs.rear.bath.window"), false, 1860, -795, -630, -711, 78);
    DoorOpening(TEXT("upstairs.rear.door"), false, 1860, -630, -35, -502, true);
    DoorOpening(TEXT("upstairs.rear.bath.door"), true, -630, 1860, 2040, 1950, true);
    Wall(true, -795, 1860, 2040);
    Wall(false, 2040, -795, -630);
    DoorOpening(TEXT("upstairs.media.door"), false, 2040, -630, -35, -498, true);
    WindowWall(TEXT("upstairs.media.window"), false, 2510, -795, -35, -413, 258);
    WindowWall(TEXT("upstairs.media.side.window"), true, -795, 2040, 2510, 2290, 172);
    Wall(true, -35, 1860, 2510);

    auto *Guards = Group(TEXT("upstairs.guards"));
    TArray<FTransform> BalusterTransforms;
    TSet<FVector> NewelLocations;
    auto Guard = [&](const FVector &Start, const FVector &End)
    {
        constexpr double RailHeight = 91;
        constexpr double BalusterPitch = 12;
        const FVector Span = End - Start;
        const FVector Center = (Start + End) / 2;
        Box(Guards, Center + FVector(0, 0, RailHeight), {Span.Size(), 8, 8}, TEXT("walnut"))
            ->SetRelativeRotation(FRotationMatrix::MakeFromXZ(Span, FVector::UpVector).Rotator());
        // The balusters seat into the steps even where the sloped rail runs above a tread.
        const double Seat = FMath::IsNearlyZero(Span.Z) ? 0. : 10.;
        const double PostHeight = RailHeight + Seat;
        const int Bays = FMath::CeilToInt(Span.Size2D() / BalusterPitch);
        for (int I = 0; I <= Bays; ++I)
        {
            const FVector Foot = Start + Span * (double(I) / Bays);
            const bool bNewel = I == 0 || I == Bays;
            if (bNewel && NewelLocations.Contains(Foot))
                continue;
            if (bNewel)
                NewelLocations.Add(Foot);
            const double Width = bNewel ? 8. : 4.;
            BalusterTransforms.Emplace(FQuat::Identity, Foot + FVector(0, 0, PostHeight / 2 - Seat),
                                      FVector(Width, Width, PostHeight) / 100);
        }
    };

    auto *MainStairs = Group(TEXT("house.stairs"));
    auto *RearStairs = Group(TEXT("house.service.stairs"));
    auto Flight = [&](ASAProp *Stairs, const FVector &Start, const FVector &Finish, double Width)
    {
        constexpr int Treads = 10;
        const FVector Travel = Finish - Start;
        const FVector Direction = Travel.GetSafeNormal2D();
        const FVector Across = FVector::CrossProduct(FVector::UpVector, Direction);
        const FRotator Slope = FRotationMatrix::MakeFromXZ(Travel, FVector::UpVector).Rotator();
        const FVector FaceNormal = Slope.RotateVector(FVector::UpVector);
        const FVector Center = (Start + Finish) / 2;
        auto *Ramp = Box(Stairs, Center - FaceNormal * 4, {Travel.Size(), Width, 8}, TEXT("wood"));
        Ramp->SetRelativeRotation(Slope);
        Ramp->SetHiddenInGame(true);
        Ramp->SetCastShadow(false);
        const double Depth = Travel.Size2D() / Treads;
        const double Rise = Travel.Z / Treads;
        for (int I = 0; I < Treads; ++I)
        {
            const FVector StepCenter = Start + Travel * ((I + .5) / Treads);
            const FRotator Yaw = Direction.Rotation();
            Box(Stairs, StepCenter - FVector(0, 0, Rise / 2), {Depth, Width, Rise}, TEXT("wood"), false)
                ->SetRelativeRotation(Yaw);
            Box(Stairs, StepCenter + FVector(0, 0, .7), {Depth + 1.5, Width + 1.5, 2.8}, TEXT("oak"), false)
                ->SetRelativeRotation(Yaw);
        }
        for (double Side : {-1., 1.})
        {
            const FVector Edge = Across * (Side * (Width / 2 - 4));
            Guard(Start + Edge, Finish + Edge);
        }
    };
    Flight(MainStairs, {490, 32, 0}, {490, 323, HalfLanding}, 102);
    Flight(MainStairs, {604, 323, HalfLanding}, {604, 32, Landing}, 104);
    Box(MainStairs, {548, 376.5, HalfLanding - 8}, {220, 107, 16}, TEXT("oak"));
    Flight(RearStairs, {1543, -116, 0}, {1757, -116, HalfLanding}, 104);
    Flight(RearStairs, {1808, -178, HalfLanding}, {1808, -446, Landing}, 102);
    Box(RearStairs, {1809, -116, HalfLanding - 8}, {104, 124, 16}, TEXT("oak"));

    Guard({443, 323, HalfLanding}, {443, 430, HalfLanding});
    Guard({443, 430, HalfLanding}, {652, 430, HalfLanding});
    Guard({652, 430, HalfLanding}, {652, 323, HalfLanding});
    Guard({537, 323, HalfLanding}, {556, 323, HalfLanding});
    Guard({438, 32, Landing}, {556, 32, Landing});
    Guard({652, 32, Landing}, {660, 32, Landing});
    Guard({660, 32, Landing}, {660, 110, Landing});
    Guard({660, 110, Landing}, {930, 110, Landing});
    Guard({438, 430, Landing}, {660, 430, Landing});
    Guard({660, 430, Landing}, {660, 470, Landing});

    Guard({1757, -164, HalfLanding}, {1761, -178, HalfLanding});
    Guard({1757, -68, HalfLanding}, {1855, -68, HalfLanding});
    Guard({1855, -68, HalfLanding}, {1855, -178, HalfLanding});
    Guard({1654, -360, Landing}, {1654, -54, Landing});
    Guard({1654, -360, Landing}, {1757, -360, Landing});
    Guard({1757, -360, Landing}, {1757, -446, Landing});
    Guard({1757, -446, Landing}, {1761, -446, Landing});
    Guard({1855, -446, Landing}, {1860, -446, Landing});
    Guard({1654, -54, Landing}, {1860, -54, Landing});

    Guard({1122, 232, Landing}, {1470, 232, Landing});
    Guard({1122, 110, Landing}, {1122, 232, Landing});
    Guard({1470, 94, Landing}, {1470, 232, Landing});
    auto *Balusters = NewObject<UInstancedStaticMeshComponent>(Guards);
    Guards->AddInstanceComponent(Balusters);
    Balusters->SetupAttachment(Guards->GetRootComponent());
    Balusters->SetStaticMesh(Cube);
    Balusters->SetMaterial(0, Materials.FindChecked(TEXT("white")).Get());
    Balusters->SetCollisionProfileName(TEXT("BlockAll"));
    Balusters->RegisterComponent();
    Balusters->AddInstances(BalusterTransforms, false);

    FurnishUpstairs();
    const FLinearColor Warm(1, .83f, .66f);
    const FLinearColor Neutral(1, .93f, .85f);
    const double Z = Ceiling - 38;
    AddFixture(TEXT("upstairs.child.light"), {215, -285, Z}, 1260, 485, Warm);
    AddFixture(TEXT("upstairs.third.light"), {215, 285, Z}, 1220, 485, Warm);
    AddFixture(TEXT("upstairs.bath.light"), {128, -20, Z}, 1000, 390, Neutral);
    AddFixture(TEXT("upstairs.hall.light"), {352, -6, Z}, 1020, 475, Warm);
    AddFixture(TEXT("upstairs.landing.light"), {548, 258, Z}, 1250, 495, Warm);
    AddFixture(TEXT("upstairs.gallery.light"), {800, -35, Z}, 1160, 470, Warm);
    AddFixture(TEXT("upstairs.family.light"), {905, 298, Z}, 1800, 850, Warm);
    AddFixture(TEXT("upstairs.master.light"), {1180, -125, Z}, 1570, 605, Warm);
    BedroomCeilingLight = Lights.Last();
    AddFixture(TEXT("upstairs.ensuite.light"), {802, -300, Z}, 1050, 430, Neutral);
    AddFixture(TEXT("upstairs.walkin.light"), {532, -370, Z}, 830, 325, Warm);
    AddFixture(TEXT("upstairs.rear.light"), {1595, -502, Z}, 1130, 460, Warm);
    AddFixture(TEXT("upstairs.service.light"), {1808, -236, Z}, 1175, 530, Warm);
    AddFixture(TEXT("upstairs.rear.hall.light"), {1932, -465, Z}, 1110, 460, Warm);
    AddFixture(TEXT("upstairs.rear.bath.light"), {1955, -712, Z}, 860, 320, Neutral);
    AddFixture(TEXT("upstairs.media.light"), {2290, -404, Z}, 1570, 660, Warm);
}

