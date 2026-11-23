#include "SAHouse.h"
#include "Components/StaticMeshComponent.h"

void ASAHouse::FurnishUpstairs()
{
    auto Place = [this](const TCHAR *Id, float X, float Y, float Height = 0, float Yaw = 0)
    {
        constexpr float DeckHeight = 342;
        return Group(Id, {X, Y, DeckHeight + Height}, Yaw);
    };
    auto BuildBed = [this](ASAProp *Bed, float Width, float Length, const TCHAR *Frame, const TCHAR *Quilt)
    {
        Box(Bed, {0, 0, 21}, {Width, Length, 22}, Frame);
        for (float Side : {-1.f, 1.f})
            for (float End : {-1.f, 1.f})
                Box(Bed, {Side * (Width / 2 - 6), End * (Length / 2 - 9), 9}, {7, 7, 18}, Frame);
        Box(Bed, {0, -Length / 2, 58}, {Width + 6, 7, 92}, Frame);
        Box(Bed, {0, -Length / 2 + 4, 72}, {Width - 12, 2, 42}, TEXT("linen"), false);
        Box(Bed, {0, 0, 41}, {Width - 4, Length - 7, 18}, TEXT("linen"), false);
        Box(Bed, {0, 21, 52}, {Width - 2, Length - 46, 7}, Quilt, false);
        Box(Bed, {0, -Length / 2 + 54, 56}, {Width - 6, 15, 3}, TEXT("linen"), false);
        for (float Side : {-1.f, 1.f})
            Box(Bed, {Side * (Width / 2 - 1), 24, 44}, {3, Length - 52, 19}, Quilt, false);
        Box(Bed, {0, Length / 2 - 2, 43}, {Width - 4, 3, 21}, Quilt, false);
    };
    auto BuildCabinet = [this](ASAProp *P, float Width, float Depth, float Height, int Drawers, const TCHAR *Wood)
    {
        Box(P, {0, 0, Height / 2 + 4}, {Width - 3, Depth - 2, Height - 8}, Wood);
        Box(P, {0, 0, Height - 1}, {Width, Depth, 3}, Wood);
        const float DrawerHeight = (Height - 17) / Drawers;
        for (int Row = 0; Row < Drawers; ++Row)
        {
            const float Z = 10 + DrawerHeight * (Row + .5f);
            Box(P, {0, Depth / 2, Z}, {Width - 7, 2, DrawerHeight - 2}, Wood, false);
            for (float Side : {-1.f, 1.f})
                Box(P, {Side * (Width > 80 ? Width / 4 : 5), Depth / 2 + 2, Z}, {3, 3, 3}, TEXT("brass"), false);
            if (Width <= 80)
                Box(P, {0, Depth / 2 + 3, Z}, {12, 2, 2}, TEXT("brass"), false);
        }
    };
    auto BuildLamp = [this](ASAProp *Lamp, FVector Foot)
    {
        Shape(Lamp, Cylinder, Foot + FVector(0, 0, 1), {16, 16, 2}, TEXT("brass"), false);
        Shape(Lamp, Cylinder, Foot + FVector(0, 0, 12), {3, 3, 22}, TEXT("brass"), false);
        auto *Shade = Shape(Lamp, Cylinder, Foot + FVector(0, 0, 28), {24, 24, 20}, TEXT("linen"), false);
        Shape(Lamp, Cylinder, Foot + FVector(0, 0, 38), {23, 23, 1}, TEXT("cream"), false);
        return Shade;
    };
    auto BuildVanity = [this, &BuildCabinet](ASAProp *P, float Width, float Depth)
    {
        BuildCabinet(P, Width, Depth, 78, 2, TEXT("oak"));
        const float Rim = (Width - 36) / 2;
        for (float Side : {-1.f, 1.f})
        {
            Box(P, {Side * (Width / 2 - Rim / 2), 0, 83}, {Rim, Depth + 4, 7}, TEXT("porcelain"));
            Box(P, {0, Side * (Depth / 2 - 4), 83}, {36, 12, 7}, TEXT("porcelain"));
        }
        Box(P, {0, 0, 78}, {36, Depth - 20, 3}, TEXT("porcelain"), false);
        Shape(P, Cylinder, {0, 2, 79.7f}, {4, 4, .5f}, TEXT("steel"), false);
        Shape(P, Cylinder, {0, -Depth / 2 + 4, 91}, {5, 5, 15}, TEXT("steel"), false);
        Box(P, {0, -Depth / 2 + 9, 98}, {4, 12, 4}, TEXT("steel"), false);
        Shape(P, Cylinder, {0, -Depth / 2 + 14, 95}, {4, 4, 5}, TEXT("steel"), false);
        Box(P, {7, -Depth / 2 + 4, 90}, {12, 4, 2}, TEXT("steel"), false);
        Box(P, {0, -Depth / 2 + 1, 142}, {Width - 4, 3, 68}, TEXT("walnut"), false);
        Box(P, {0, -Depth / 2 + 3, 142}, {Width - 11, 1, 60}, TEXT("mirror"), false);
        Shape(P, Cylinder, {Width / 2 - 8, 1, 92}, {7, 7, 12}, TEXT("sage"), false);
        Box(P, {Width / 2 - 7, 1, 99}, {6, 2, 2}, TEXT("steel"), false);
    };
    auto BuildToilet = [this](ASAProp *P)
    {
        Shape(P, Cylinder, {5, 0, 12}, {25, 29, 24}, TEXT("porcelain"));
        Shape(P, Sphere, {10, 0, 30}, {51, 37, 29}, TEXT("porcelain"));
        Shape(P, Sphere, {10, 0, 43}, {49, 36, 7}, TEXT("white"), false);
        Box(P, {-20, 0, 50}, {19, 42, 48}, TEXT("porcelain"));
        Box(P, {-20, 0, 75}, {21, 44, 4}, TEXT("porcelain"), false);
        Box(P, {-9, -13, 68}, {3, 7, 3}, TEXT("steel"), false);
    };

    auto *ChildBed = Place(TEXT("upstairs.child.bed"), 142, -346);
    BuildBed(ChildBed, 104, 196, TEXT("white"), TEXT("blush"));
    Shape(ChildBed, Sphere, {0, -66, 53}, {69, 37, 12}, TEXT("linen"), false);
    auto *ChildStand = Place(TEXT("upstairs.child.nightstand"), 235, -408);
    BuildCabinet(ChildStand, 43, 38, 62, 2, TEXT("oak"));
    BuildLamp(ChildStand, {5, -3, 63});

    auto *Desk = Place(TEXT("upstairs.child.desk"), 142, -179, 0, 180);
    Box(Desk, {0, 0, 70}, {118, 55, 5}, TEXT("oak"));
    for (float X : {-53.f, 53.f})
        Box(Desk, {X, 0, 34}, {5, 48, 68}, TEXT("oak"));
    Box(Desk, {0, -17, 54}, {105, 5, 13}, TEXT("oak"), false);
    Box(Desk, {-21, 7, 75}, {27, 20, 3}, TEXT("blue"), false)->SetRelativeRotation({0, -18, 0});
    Box(Desk, {16, 6, 73}, {22, 29, 1}, TEXT("white"), false)->SetRelativeRotation({0, 12, 0});
    for (int Pencil = 0; Pencil < 4; ++Pencil)
        Box(Desk, {12.f + Pencil * 3, 7, 74}, {1, 14, 1}, Pencil % 2 == 0 ? TEXT("red") : TEXT("yellow"), false)
            ->SetRelativeRotation({0, Pencil * 5.f, 0});
    BuildLamp(Desk, {40, -12, 73});
    auto *Chair = Place(TEXT("upstairs.child.chair"), 142, -234, 0, 180);
    Box(Chair, {0, 0, 39}, {42, 42, 5}, TEXT("oak"));
    Box(Chair, {0, 17, 62}, {41, 6, 43}, TEXT("oak"));
    Shape(Chair, Sphere, {0, -1, 41}, {39, 39, 8}, TEXT("blush"), false);
    for (float X : {-16.f, 16.f})
        for (float Y : {-16.f, 16.f})
            Box(Chair, {X, Y, 19}, {4, 4, 38}, TEXT("oak"));
    auto *Drawing = Place(TEXT("upstairs.child.drawing"), 14, -298, 133);
    Box(Drawing, {0, 0, 0}, {5, 43, 38}, TEXT("oak"), false);
    Box(Drawing, {3, 0, 0}, {2, 36, 31}, TEXT("white"), false);
    Shape(Drawing, Sphere, {4.5f, 8, 7}, {1, 9, 9}, TEXT("yellow"), false);
    Shape(Drawing, Sphere, {4.5f, -7, -9}, {1, 21, 12}, TEXT("sage"), false);
    Shape(Drawing, Sphere, {5, 7, -9}, {1, 18, 10}, TEXT("blue"), false);

    auto *Play = Place(TEXT("upstairs.child.play"), 258, -348);
    Box(Play, {0, 0, .7f}, {193, 167, 1.4f}, TEXT("jute"), false);
    Box(Play, {0, 0, 1.5f}, {176, 150, .6f}, TEXT("sage"), false);
    for (int Block = 0; Block < 5; ++Block)
        Box(Play, {-69.f + Block * 13, -52.f + (Block % 2) * 9, 5}, {9, 9, 9},
            Block % 2 == 0 ? TEXT("red") : TEXT("oak"), false)->SetRelativeRotation({0, Block * 19.f, 0});
    auto *Shelf = Place(TEXT("upstairs.child.shelf"), 29, -217, 0, -90);
    for (float X : {-53.f, 53.f})
        Box(Shelf, {X, 0, 69}, {4, 23, 138}, TEXT("oak"));
    Box(Shelf, {0, -10, 69}, {102, 3, 132}, TEXT("oak"));
    for (float Z : {8.f, 51.f, 94.f, 137.f})
        Box(Shelf, {0, 0, Z}, {108, 26, 4}, TEXT("oak"));
    for (int Book = 0; Book < 14; ++Book)
    {
        const TCHAR *Colors[] = {TEXT("blush"), TEXT("blue"), TEXT("sage"), TEXT("linen")};
        const float ShelfHeight = Book >= 7 ? 94 : 51;
        const float BookHeight = 17.f + (Book * 7) % 14;
        Box(Shelf, {-43.f + (Book % 7) * 8, 1, ShelfHeight + 2 + BookHeight / 2},
            {6, 18, BookHeight}, Colors[Book % UE_ARRAY_COUNT(Colors)], false);
    }
    Box(Shelf, {23, 0, 24}, {36, 20, 28}, TEXT("jute"), false);

    auto *ChildDresser = Place(TEXT("upstairs.child.dresser"), 385, -385, 0, 90);
    BuildCabinet(ChildDresser, 108, 51, 96, 3, TEXT("white"));
    auto *Doll = Place(TEXT("upstairs.child.doll"), 377, -385, 97, 180);
    for (float Y : {-5.f, 5.f})
    {
        Shape(Doll, Sphere, {3, Y, 3}, {10, 7, 6}, TEXT("brown"), false);
        Shape(Doll, Sphere, {0, Y, 9}, {6, 6, 15}, TEXT("linen"), false);
        Shape(Doll, Sphere, {0, Y * 2.3f, 22}, {7, 7, 17}, TEXT("cream"), false)
            ->SetRelativeRotation({0, 0, FMath::Sign(Y) * -20.f});
        Box(Doll, {7.8f, Y * .7f, 35}, {1, 1.8f, 2}, TEXT("black"), false);
    }
    Shape(Doll, Sphere, {0, 0, 18}, {15, 20, 20}, TEXT("blue"), false);
    Shape(Doll, Sphere, {-1, 0, 34}, {18, 17, 19}, TEXT("brown"), false);
    Shape(Doll, Sphere, {3, 0, 33}, {11, 13, 14}, TEXT("cream"), false);
    Box(Doll, {9, 0, 30}, {1, 4, 1.2f}, TEXT("blush"), false);
    Box(Doll, {5, 1, 43}, {3, 16, 4}, TEXT("red"), false);
    Box(Doll, {7, 0, 23}, {1, 13, 4}, TEXT("linen"), false);

    auto *ThirdBed = Place(TEXT("upstairs.third.bed"), 150, 351, 0, 180);
    BuildBed(ThirdBed, 114, 196, TEXT("walnut"), TEXT("slate"));
    Shape(ThirdBed, Sphere, {0, -66, 53}, {78, 37, 12}, TEXT("linen"), false);
    auto *ThirdStand = Place(TEXT("upstairs.third.nightstand"), 236, 407, 0, 180);
    BuildCabinet(ThirdStand, 44, 42, 62, 2, TEXT("walnut"));
    BuildLamp(ThirdStand, {0, -3, 63});
    auto *ThirdDresser = Place(TEXT("upstairs.third.dresser"), 390, 399, 0, 90);
    BuildCabinet(ThirdDresser, 107, 51, 92, 3, TEXT("oak"));
    Box(ThirdDresser, {-24, 1, 94}, {31, 25, 4}, TEXT("leather"), false);
    Shape(ThirdDresser, Sphere, {25, -3, 104}, {24, 17, 24}, TEXT("porcelain"), false);
    auto *ThirdRug = Place(TEXT("upstairs.third.rug"), 265, 307);
    Box(ThirdRug, {0, 0, .6}, {198, 150, 1.2}, TEXT("jute"), false);
    Box(ThirdRug, {0, 0, 1.3}, {181, 134, .5}, TEXT("slate"), false);

    BuildVanity(Place(TEXT("upstairs.bath.vanity"), 153, -105), 91, 48);
    BuildToilet(Place(TEXT("upstairs.bath.toilet"), 63, -99, 0, 90));
    auto *Tub = Place(TEXT("upstairs.bath.tub"), 45, 15, 0, 90);
    Box(Tub, {0, 0, 8}, {164, 63, 16}, TEXT("porcelain"));
    for (float Side : {-1.f, 1.f})
    {
        Box(Tub, {0, Side * 31, 27}, {171, 7, 52}, TEXT("porcelain"));
        Box(Tub, {0, Side * 30, 54}, {172, 11, 5}, TEXT("porcelain"));
        Box(Tub, {Side * 82, 0, 27}, {8, 62, 52}, TEXT("porcelain"));
        Box(Tub, {Side * 82, 0, 54}, {11, 70, 5}, TEXT("porcelain"));
    }
    Shape(Tub, Cylinder, {64, 0, 16.4f}, {5, 5, .6f}, TEXT("steel"), false);
    Shape(Tub, Cylinder, {81, 0, 62}, {6, 6, 14}, TEXT("steel"), false);
    Box(Tub, {74, 0, 68}, {15, 6, 5}, TEXT("steel"), false);
    for (float Y : {-12.f, 12.f})
        Shape(Tub, Cylinder, {81, Y, 58}, {7, 7, 7}, TEXT("steel"), false);
    Box(Tub, {-47, -25, 57}, {27, 19, 2}, TEXT("oak"), false);
    Shape(Tub, Sphere, {-47, -25, 59}, {12, 8, 4}, TEXT("sage"), false);

    auto *Rail = Place(TEXT("upstairs.bath.rail"), 120, 127, 123);
    for (float X : {-38.f, 38.f})
        Box(Rail, {X, 0, 0}, {5, 10, 5}, TEXT("steel"), false);
    Shape(Rail, Cylinder, {0, -5, 0}, {3, 3, 79}, TEXT("steel"), false)->SetRelativeRotation({90, 0, 0});
    auto *Towel = Place(TEXT("upstairs.bath.towel"), 120, 121, 122);
    Box(Towel, {0, .5f, -23}, {56, 2, 47}, TEXT("linen"), false);
    for (int Fold = 0; Fold < 8; ++Fold)
        Shape(Towel, Sphere, {-24.5f + Fold * 7, -1, -22}, {8, 4, 47 + (Fold % 2) * 2.f}, TEXT("linen"), false);
    Box(Towel, {0, -2.5f, -40}, {55, 1.5f, 3}, TEXT("cream"), false);
    auto *BathMat = Place(TEXT("upstairs.bath.mat"), 149, 27);
    Box(BathMat, {0, 0, 1}, {107, 38, 2}, TEXT("sage"), false);

    auto *MasterBed = Place(TEXT("upstairs.master.bed"), 1277, -278);
    BuildBed(MasterBed, 150, 190, TEXT("walnut"), TEXT("sage"));
    for (float X : {-35.f, 35.f})
        Shape(MasterBed, Sphere, {X, -66, 53}, {63, 39, 12}, TEXT("linen"), false);
    Box(MasterBed, {0, 66, 57}, {147, 37, 3}, TEXT("blush"), false);
    Box(MasterBed, {72, 66, 39}, {3, 37, 36}, TEXT("blush"), false);
    auto *MasterStand = Place(TEXT("upstairs.master.nightstand"), 1378, -343);
    BuildCabinet(MasterStand, 46, 42, 68, 2, TEXT("walnut"));
    BedsideShade = BuildLamp(MasterStand, {7, -9, 69});
    BedsideShade->SetCastShadow(false);
    auto *Photo = Place(TEXT("upstairs.master.photo"), 1369, -332, 80, 90);
    Box(Photo, {0, 0, 0}, {2.5f, 21, 23}, TEXT("walnut"), false)->SetRelativeRotation({-9, 0, 0});
    Box(Photo, {1.5f, 0, 0}, {1, 17, 19}, TEXT("linen"), false)->SetRelativeRotation({-9, 0, 0});
    Box(Photo, {2.5f, 0, 0}, {.5f, 12, 14}, TEXT("blue"), false)->SetRelativeRotation({-9, 0, 0});
    for (float Side : {-1.f, 1.f})
    {
        Shape(Photo, Sphere, {3, Side * 2.8f, .3f}, {.8f, 3, 4}, TEXT("cream"), false);
        Box(Photo, {3.2f, Side * 2.8f, -3.5f}, {.6f, 4, 5}, TEXT("slate"), false);
    }
    Box(Photo, {-4, 0, -6}, {2, 9, 11}, TEXT("walnut"), false)->SetRelativeRotation({35, 0, 0});

    auto *Bench = Place(TEXT("upstairs.master.bench"), 1277, -153);
    Box(Bench, {0, 0, 39}, {106, 33, 7}, TEXT("linen"));
    Box(Bench, {0, 0, 33}, {104, 31, 5}, TEXT("walnut"));
    for (float X : {-46.f, 46.f})
        for (float Y : {-12.f, 12.f})
            Box(Bench, {X, Y, 16}, {4.5f, 4.5f, 32}, TEXT("walnut"));
    auto *MasterDresser = Place(TEXT("upstairs.master.dresser"), 1230, -9, 0, 180);
    BuildCabinet(MasterDresser, 102, 41, 92, 3, TEXT("walnut"));
    Box(MasterDresser, {-28, 0, 94}, {25, 17, 3}, TEXT("leather"), false)->SetRelativeRotation({0, -12, 0});
    Shape(MasterDresser, Sphere, {32, 0, 97}, {25, 19, 8}, TEXT("porcelain"), false);
    auto *Shower = Place(TEXT("upstairs.ensuite.shower"), 730, -403);
    Box(Shower, {0, 0, 5}, {107, 83, 10}, TEXT("porcelain"));
    Box(Shower, {0, 41, 8}, {107, 5, 5}, TEXT("porcelain"));
    Shape(Shower, Cylinder, {0, 9, 10.4f}, {8, 8, .7f}, TEXT("steel"), false);
    Shape(Shower, Cylinder, {-44, -35, 166}, {4, 4, 43}, TEXT("steel"), false);
    Box(Shower, {-44, -26, 188}, {4, 21, 4}, TEXT("steel"), false);
    Shape(Shower, Cylinder, {-44, -16, 185}, {14, 14, 4}, TEXT("steel"), false);
    Shape(Shower, Cylinder, {-44, -37, 106}, {11, 11, 3}, TEXT("steel"), false)
        ->SetRelativeRotation({0, 0, 90});
    Shape(Shower, Cylinder, {0, 41, 211}, {3, 3, 107}, TEXT("steel"), false)
        ->SetRelativeRotation({90, 0, 0});
    for (int Fold = 0; Fold < 7; ++Fold)
    {
        const float X = -49.f + Fold * 6;
        Shape(Shower, Sphere, {X, 41, 112}, {9, 5 + (Fold % 2) * 2.f, 190}, TEXT("linen"), false);
        Box(Shower, {X, 41, 208}, {1.6f, 2, 6}, TEXT("steel"), false);
    }
    BuildVanity(Place(TEXT("upstairs.ensuite.vanity"), 832, -192, 0, 180), 107, 43);
    BuildToilet(Place(TEXT("upstairs.ensuite.toilet"), 875, -430, 0, 180));
    auto *EnsuiteMat = Place(TEXT("upstairs.ensuite.mat"), 801, -285);
    Box(EnsuiteMat, {0, 0, .8f}, {94, 37, 1.6f}, TEXT("sage"), false);

    auto *WalkIn = Place(TEXT("upstairs.walkin.shelves"), 543, -436);
    for (double Height : {8., 47., 85., 124., 177.})
        Box(WalkIn, {0, 0, Height}, {177, 49, 4}, TEXT("oak"));
    for (double Side : {-88., 88.})
        Box(WalkIn, {Side, 0, 90}, {5, 49, 180}, TEXT("walnut"));
    for (double X : {-56., -22., 12., 46.})
        Box(WalkIn, {X, 0, 97}, {30, 39, 19}, TEXT("linen"), false);

    auto *LandingBench = Place(TEXT("upstairs.gallery.bench"), 746, -135);
    Box(LandingBench, {0, 0, 23}, {110, 40, 46}, TEXT("walnut"));
    Box(LandingBench, {0, 0, 48}, {106, 39, 9}, TEXT("sage"), false);

    auto *MediaBed = Place(TEXT("upstairs.media.bed"), 2398, -620);
    BuildBed(MediaBed, 146, 194, TEXT("walnut"), TEXT("slate"));
    for (double Side : {-35., 35.})
        Shape(MediaBed, Sphere, {Side, -64, 53}, {61, 38, 12}, TEXT("linen"), false);
    auto *MediaStand = Place(TEXT("upstairs.media.nightstand"), 2286, -689);
    BuildCabinet(MediaStand, 48, 43, 66, 2, TEXT("walnut"));
    BuildLamp(MediaStand, {3, -1, 67});
    auto *MediaConsole = Place(TEXT("upstairs.media.console"), 2270, -100, 0, 180);
    BuildCabinet(MediaConsole, 161, 45, 54, 2, TEXT("oak"));
    Box(MediaConsole, {0, 0, 68}, {87, 10, 7}, TEXT("slate"), false);
    Box(MediaConsole, {0, 0, 107}, {137, 11, 76}, TEXT("black"));
    Box(MediaConsole, {0, 6, 107}, {129, 2, 68}, TEXT("night"), false);
    auto *MediaSeat = Place(TEXT("upstairs.media.seat"), 2270, -283);
    Box(MediaSeat, {0, 0, 31}, {171, 74, 58}, TEXT("canvas"));
    Box(MediaSeat, {0, -31, 71}, {171, 17, 70}, TEXT("canvas"));
    for (double X : {-55., 0., 55.})
        Box(MediaSeat, {X, 4, 61}, {53, 57, 8}, TEXT("linen"), false);

    BuildVanity(Place(TEXT("upstairs.rear.bath.vanity"), 1911, -758), 72, 43);
    BuildToilet(Place(TEXT("upstairs.rear.bath.toilet"), 2002, -758, 0, 180));
    auto *RearMat = Place(TEXT("upstairs.rear.bath.mat"), 1950, -701);
    Box(RearMat, {0, 0, .8}, {93, 36, 1.6}, TEXT("slate"), false);
}
