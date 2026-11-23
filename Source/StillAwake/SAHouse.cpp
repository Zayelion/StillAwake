#include "SAHouse.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

ASAProp::ASAProp()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}
void ASAProp::Visible(bool Value)
{
    SetActorHiddenInGame(!Value);
    SetActorEnableCollision(Value);
}
void ASAProp::Capture()
{
    Baseline = RootComponent->GetRelativeTransform();
    bBaselineVisible = !IsHidden();
    if (Words)
        BaselineText = Words->Text.ToString();
    TArray<UStaticMeshComponent *> Meshes;
    GetComponents(Meshes);
    BaselineMaterials.Reset();
    for (auto *M : Meshes)
        BaselineMaterials.Add(M->GetMaterial(0));
}
void ASAProp::Tint(const FLinearColor &Color)
{
    TArray<UStaticMeshComponent *> Meshes;
    GetComponents(Meshes);
    for (auto *M : Meshes)
    {
        auto *Material = M->GetMaterial(0);
        auto *Current = Cast<UMaterialInstanceDynamic>(Material);
        auto *D = UMaterialInstanceDynamic::Create(Current ? Current->Parent.Get() : Material, this);
        if (Current)
            D->CopyParameterOverrides(Current);
        D->SetVectorParameterValue(TEXT("Tint"), Color);
        M->SetMaterial(0, D);
    }
    bTinted = true;
}
void ASAProp::Reset()
{
    RootComponent->SetRelativeTransform(Baseline);
    Visible(bBaselineVisible);
    if (Words)
        Words->SetText(FText::FromString(BaselineText));
    TArray<UStaticMeshComponent *> Meshes;
    GetComponents(Meshes);
    for (int I = 0; I < Meshes.Num() && I < BaselineMaterials.Num(); ++I)
        Meshes[I]->SetMaterial(0, BaselineMaterials[I]);
    bTinted = false;
}
ASAHouse::ASAHouse()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HouseRoot"));
}
ASAProp *ASAHouse::Group(const FString &Id, FVector Position, float Yaw)
{
    auto *P = GetWorld()->SpawnActor<ASAProp>(Position, FRotator(0, Yaw, 0));
    Groups.Add(P);
    Props.Add(Id, P);
    P->Tags.Add(FName(*Id));
    return P;
}
UStaticMeshComponent *ASAHouse::Shape(ASAProp *P, UStaticMesh *Mesh, FVector Position, FVector Size, const FString &Mat,
                                      bool Solid)
{
    auto *C = NewObject<UStaticMeshComponent>(P);
    P->AddInstanceComponent(C);
    C->SetupAttachment(P->GetRootComponent());
    C->SetMobility(EComponentMobility::Movable);
    C->SetStaticMesh(Mesh);
    C->SetRelativeLocation(Position);
    C->SetRelativeScale3D(Size / 100);
    C->SetCollisionProfileName(Solid ? TEXT("BlockAll") : TEXT("NoCollision"));
    C->SetMaterial(0, Materials.FindChecked(Mat));
    C->RegisterComponent();
    return C;
}
UStaticMeshComponent *ASAHouse::Box(ASAProp *P, FVector Pos, FVector Size, const FString &Mat, bool Solid)
{
    return Shape(P, Cube, Pos, Size, Mat, Solid);
}
UTextRenderComponent *ASAHouse::Label(ASAProp *P, const FString &Text, FVector Pos, float Size, FRotator Rotation)
{
    auto *C = NewObject<UTextRenderComponent>(P);
    P->AddInstanceComponent(C);
    C->SetupAttachment(P->GetRootComponent());
    C->SetRelativeLocation(Pos);
    C->SetRelativeRotation(Rotation);
    C->SetHorizontalAlignment(EHTA_Center);
    C->SetVerticalAlignment(EVRTA_TextCenter);
    C->SetWorldSize(Size);
    C->SetTextRenderColor(FColor(230, 220, 193));
    C->SetText(FText::FromString(Text));
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->RegisterComponent();
    P->Words = C;
    return C;
}
void ASAHouse::TileFloor(FVector Center, FVector Size, float Yaw)
{
    auto *P = Group(FString::Printf(TEXT("floor.%d"), Groups.Num()), Center, Yaw);
    Box(P, {0, 0, -6}, {Size.X, Size.Y, 12}, TEXT("grout"));
    const int CountX = FMath::CeilToInt(Size.X / 50);
    const int CountY = FMath::CeilToInt(Size.Y / 50);
    const double TileX = Size.X / CountX;
    const double TileY = Size.Y / CountY;
    for (int Shade = 0; Shade < 3; ++Shade)
    {
        auto *C = NewObject<UInstancedStaticMeshComponent>(P);
        P->AddInstanceComponent(C);
        C->SetupAttachment(P->GetRootComponent());
        C->SetStaticMesh(Cube);
        C->SetMaterial(0, Materials.FindChecked(FString::Printf(TEXT("tile%d"), Shade)));
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->RegisterComponent();
        for (int X = 0; X < CountX; ++X)
            for (int Y = 0; Y < CountY; ++Y)
                if ((X * 17 + Y * 7) % 3 == Shade)
                    C->AddInstance(FTransform(FQuat::Identity,
                                              FVector(-Size.X / 2 + (X + .5) * TileX,
                                                      -Size.Y / 2 + (Y + .5) * TileY, .3),
                                              FVector((TileX - 1.5) / 100, (TileY - 1.5) / 100, .008)),
                                   false);
    }
    FRandomStream Dirt(44 + int(Center.X + Center.Y));
    for (int I = 0; I < 15; ++I)
        Shape(P, Sphere,
              {Dirt.FRandRange(-Size.X * .45f, Size.X * .45f), Dirt.FRandRange(-Size.Y * .45f, Size.Y * .45f), .9f},
              {Dirt.FRandRange(10, 28), Dirt.FRandRange(6, 16), .15f}, TEXT("dirt"), false);
}
void ASAHouse::Table(const FString &Id, FVector Position, FVector Size)
{
    auto *P = Group(Id, Position);
    Box(P, {0, 0, 76}, {Size.X, Size.Y, 7}, TEXT("wood"));
    for (float X : {-Size.X / 2 + 8, Size.X / 2 - 8})
        for (float Y : {-Size.Y / 2 + 8, Size.Y / 2 - 8})
            Box(P, {X, Y, 36}, {6, 6, 72}, TEXT("wood"));
}
void ASAHouse::Door(const FString &Id, FVector Hinge, float Yaw)
{
    AddRoomDoor(Id, Hinge, Yaw);
    if (Id != TEXT("front.door"))
        return;
    auto *Frame = Props.FindChecked(Id + TEXT(".frame")).Get();
    Box(Frame, {-11, 48, 235}, {2, 49, 23}, TEXT("wood"), false);
    Counters.Add(Label(Frame, TEXT("3"), {-13, 48, 235}, 16, {0, 180, 0}));
}
void ASAHouse::Window(float X, bool Opening)
{
    auto *Frame = Group(FString::Printf(TEXT("window.frame.%d"), int(X)), {X, 465, 165}, -90);
    Box(Frame, {-8, 0, 0}, {4, 160, 150}, TEXT("night"), false);
    for (float Y : {-80.f, 80.f})
        Box(Frame, {0, Y, 0}, {18, 8, 158}, TEXT("white"));
    for (float Z : {-78.f, 78.f})
        Box(Frame, {0, 0, Z}, {18, 168, 8}, TEXT("white"));
    auto *Sash = Group(Opening ? TEXT("living.window") : TEXT("living.fixedWindow"), {X - 72, 456, 165}, -90);
    Box(Sash, {0, 72, 0}, {5, 142, 142}, TEXT("glass"));
    for (float Y : {3.f, 141.f})
        Box(Sash, {4, Y, 0}, {6, 5, 142}, TEXT("white"));
    for (float Z : {-69.f, 0.f, 69.f})
        Box(Sash, {4, 72, Z}, {6, 142, 5}, TEXT("white"));
    Box(Sash, {8, 72, 0}, {8, 5, 138}, TEXT("white"));
    Box(Frame, {20, 0, 105}, {5, 225, 5}, TEXT("steel"), false);
    for (float Side : {-1.f, 1.f})
        for (int I = 0; I < 7; ++I)
            Shape(Frame, Cylinder, {25, Side * (95 + I * 6), -5}, {12, 12, 210}, TEXT("curtain"), false);
}
void ASAHouse::Painting(float X, bool White)
{
    auto *Frame = Group(FString::Printf(TEXT("painting.%d"), int(X)), {X, -100, 145}, 90);
    Box(Frame, {0, 0, 0}, {9, 70, 85}, TEXT("wood"));
    Box(Frame, {5, 0, 0}, {2, 61, 76}, TEXT("canvas"), false);
    auto *Cat = Group(White ? TEXT("foyer.whitecat") : TEXT("foyer.blackcat"), {X, -93, 145}, 90);
    FString Color = White ? TEXT("white") : TEXT("black");
    Shape(Cat, Sphere, {0, 0, -11}, {3, 29, 43}, Color, false);
    Shape(Cat, Sphere, {1, 0, 15}, {3, 27, 24}, Color, false);
    for (float Y : {-9.f, 9.f})
        Shape(Cat, Cone, {1, Y, 29}, {3, 10, 15}, Color, false);
    Shape(Cat, Sphere, {0, 19, -26}, {3, 26, 8}, Color, false);
    // Eyes belong to the frame so the white-cat recolor preserves them.
    for (float Y : {-6.f, 6.f})
        Shape(Frame, Sphere, {10, Y, 16}, {2, 3, 4}, TEXT("gold"), false);
}
void ASAHouse::Build()
{
    Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
    Surface = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StillAwake/M_HouseTextured.M_HouseTextured"));
    if (!Surface)
        Surface = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StillAwake/M_HouseDetailed.M_HouseDetailed"));
    if (!Surface)
        Surface = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StillAwake/M_House.M_House"));
    check(Cube && Sphere && Cylinder && Cone && Surface);
    enum class Finish
    {
        Plaster, Paint, Oak, Walnut,
        Ceramic, Grout, Soil, Steel,
        Brass, Linen, Carpet, Paper,
        Glass, Water, Leather, Porcelain
    };
    // Tile order and reference luminance match HouseSurfaces.png.
    constexpr float Midtones[] = {.778f, .763f, .522f, .271f, .610f, .473f, .342f, .475f,
                                  .426f, .687f, .489f, .739f, .250f, .415f, .250f, .776f};
    auto AddMaterial = [&](const TCHAR *Name, Finish Tile, FLinearColor Color, float Roughness = .74f,
                           float Detail = .72f, FVector2D Period = {44, 44}, float Metallic = 0, float Emission = 0)
    {
        auto *M = UMaterialInstanceDynamic::Create(Surface, this);
        const int Index = static_cast<int>(Tile);
        M->SetVectorParameterValue(TEXT("Tint"), Color);
        M->SetVectorParameterValue(TEXT("AtlasTile"), {float(Index % 4), float(Index / 4), Midtones[Index], 0});
        M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
        M->SetScalarParameterValue(TEXT("SurfaceDetail"), Detail);
        M->SetVectorParameterValue(TEXT("GrainScale"), {float(1.0 / Period.X), float(1.0 / Period.Y), 0, 0});
        M->SetScalarParameterValue(TEXT("Metallic"), Metallic);
        M->SetScalarParameterValue(TEXT("Emission"), Emission);
        Materials.Add(Name, M);
    };
    const FVector2D WoodGrain(54, 142);
    AddMaterial(TEXT("cream"), Finish::Plaster, {.62f, .55f, .43f}, .89f, .83f, {58, 58});
    AddMaterial(TEXT("white"), Finish::Paint, {.85f, .83f, .76f}, .65f, .85f, {31, 78});
    AddMaterial(TEXT("ceiling"), Finish::Plaster, {.72f, .69f, .62f}, .9f, .59f, {42, 42});
    AddMaterial(TEXT("wood"), Finish::Walnut, {.16f, .082f, .039f}, .57f, 1.f, WoodGrain);
    AddMaterial(TEXT("doorInset"), Finish::Oak, {.22f, .128f, .071f}, .56f, .95f, WoodGrain);
    AddMaterial(TEXT("tile0"), Finish::Ceramic, {.64f, .63f, .57f});
    AddMaterial(TEXT("tile1"), Finish::Ceramic, {.71f, .70f, .65f});
    AddMaterial(TEXT("tile2"), Finish::Ceramic, {.59f, .59f, .54f});
    AddMaterial(TEXT("grout"), Finish::Grout, {.20f, .20f, .18f}, .93f, .68f, {26, 26});
    AddMaterial(TEXT("dirt"), Finish::Soil, {.28f, .26f, .20f}, .96f, .88f, {73, 73});
    AddMaterial(TEXT("black"), Finish::Linen, {.012f, .014f, .018f}, .88f, .6f, {19, 19});
    AddMaterial(TEXT("brown"), Finish::Walnut, {.16f, .075f, .035f}, .68f, .89f, WoodGrain);
    AddMaterial(TEXT("curtain"), Finish::Linen, {.23f, .24f, .25f}, .92f, .74f, {24, 28});
    AddMaterial(TEXT("steel"), Finish::Steel, {.35f, .39f, .4f}, .38f, .63f, {36, 24}, .65f);
    AddMaterial(TEXT("blue"), Finish::Paint, {.025f, .08f, .28f});
    AddMaterial(TEXT("yellow"), Finish::Paint, {.8f, .52f, .015f});
    AddMaterial(TEXT("brass"), Finish::Brass, {.48f, .31f, .1f}, .37f, .78f, {24, 24}, .65f);
    AddMaterial(TEXT("gold"), Finish::Brass, {.6f, .47f, .12f}, .42f, .69f, {23, 23}, .45f);
    AddMaterial(TEXT("canvas"), Finish::Paper, {.26f, .23f, .18f}, .93f, .81f, {25, 25});
    AddMaterial(TEXT("night"), Finish::Glass, {.004f, .009f, .018f}, .81f, .47f);
    AddMaterial(TEXT("glass"), Finish::Glass, {.035f, .065f, .10f}, .19f, .85f, {79, 79}, .3f, .12f);
    AddMaterial(TEXT("water"), Finish::Water, {.3f, .48f, .56f}, .17f, .76f, {19, 20});
    AddMaterial(TEXT("red"), Finish::Paint, {.5f, .03f, .025f});
    AddMaterial(TEXT("lampGlass"), Finish::Porcelain, {.94f, .79f, .59f}, .45f, .64f, {40, 40}, 0, .32f);
    AddMaterial(TEXT("leather"), Finish::Leather, {.215f, .105f, .063f}, .68f, .88f, {21, 21});
    AddMaterial(TEXT("stainless"), Finish::Steel, {.48f, .53f, .55f}, .34f, .75f, {76, 32}, .72f);
    AddMaterial(TEXT("rug"), Finish::Carpet, {.15f, .17f, .14f}, .95f, .67f, {34, 34});
    AddMaterial(TEXT("jute"), Finish::Carpet, {.42f, .35f, .23f}, .95f, .76f, {26, 26});
    AddMaterial(TEXT("walnut"), Finish::Walnut, {.115f, .06f, .035f}, .59f, 1.f, WoodGrain);
    AddMaterial(TEXT("oak"), Finish::Oak, {.34f, .214f, .123f}, .56f, 1.f, WoodGrain);
    AddMaterial(TEXT("linen"), Finish::Linen, {.67f, .63f, .55f}, .94f, .76f, {25, 25});
    AddMaterial(TEXT("sage"), Finish::Linen, {.255f, .34f, .25f}, .88f, .72f, {28, 28});
    AddMaterial(TEXT("slate"), Finish::Paint, {.165f, .23f, .285f}, .72f);
    AddMaterial(TEXT("porcelain"), Finish::Porcelain, {.82f, .84f, .80f}, .23f, .92f, {47, 47});
    AddMaterial(TEXT("mirror"), Finish::Glass, {.38f, .47f, .54f}, .16f, .63f, {53, 53}, .58f);
    AddMaterial(TEXT("blush"), Finish::Linen, {.53f, .255f, .20f}, .87f, .75f, {24, 24});
    AddMaterial(TEXT("stickerPaper"), Finish::Paper, {.93f, .71f, .31f}, .9f, .67f, {18, 18}, 0, .36f);
    for (int I = 0; I < 4; ++I)
    {
        float V = .12f + I * .23f;
        AddMaterial(*FString::Printf(TEXT("static%d"), I), Finish::Glass, {V, V, V}, .5f, .47f, {16, 16});
    }
    BuildGroundFloor();
    Furnish();
    DetailFirstFloor();
    BuildUpstairs();
    BuildSleepScene();
    for (auto P : Groups)
    {
        if (!P->GetAttachParentActor())
            P->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    }
    for (auto P : Groups)
        P->Capture();
    Reset();
}
void ASAHouse::Furnish()
{
    Table(TEXT("porch.table"), {-95, 154, 0}, {90, 60, 0});
    auto *Paper = Group(TEXT("porch.newspaper"), {-95, 154, 81});
    Box(Paper, {0, 0, 0}, {62, 43, 2}, TEXT("white"));
    Label(Paper, TEXT("TRYING TO\nKEEP SANE"), {0, 0, 1.5f}, 5, {90, 0, 0})->SetTextRenderColor(FColor(25, 22, 20));
    Table(TEXT("backporch.table"), {1440, 390, 0}, {60, 90, 0});
    auto *BackPaper = Group(TEXT("backporch.newspaper"), {1440, 390, 81}, -90);
    Box(BackPaper, {0, 0, 0}, {62, 43, 2}, TEXT("white"));
    Label(BackPaper, TEXT("TRYING TO\nKEEP SANE"), {0, 0, 1.5f}, 5, {90, 0, 0})->SetTextRenderColor(FColor(25, 22, 20));
    Painting(225, true);
    Painting(397, false);
    Table(TEXT("foyer.table"), {282, -80, 0}, {172, 43, 0});
    auto *Book = Group(TEXT("foyer.book"), {301, -83, 82});
    Box(Book, {0, 0, 0}, {34, 24, 7}, TEXT("blue"));
    Box(Book, {0, 0, 0}, {30, 25, 4}, TEXT("white"));
    auto *Clock = Group(TEXT("foyer.clock"), {336, -95, 118}, 90);
    Shape(Clock, Cylinder, {0, 0, 0}, {40, 40, 9}, TEXT("wood"), false)->SetRelativeRotation({90, 0, 0});
    Shape(Clock, Sphere, {5, 0, 0}, {3, 33, 33}, TEXT("white"), false);
    Box(Clock, {8, 0, 6}, {2, 2, 13}, TEXT("black"), false);
    Box(Clock, {8, 5, 0}, {2, 12, 2}, TEXT("black"), false);
    Label(Clock, TEXT("12"), {10, 0, 13}, 5)->SetTextRenderColor(FColor::Black);
    auto *Hooks = Group(TEXT("foyer.hooks"), {150, 101, 195});
    Box(Hooks, {0, 0, 0}, {109, 8, 16}, TEXT("wood"));
    for (float X : {-42.f, -14.f, 14.f, 42.f})
    {
        Box(Hooks, {X, -10, -5}, {4, 18, 4}, TEXT("steel"), false);
        Shape(Hooks, Sphere, {X, -18, 0}, {6, 6, 10}, TEXT("steel"), false);
    }
    auto *Coat = Group(TEXT("foyer.raincoat"), {150, 91, 135});
    Shape(Coat, Sphere, {0, 0, 3}, {60, 18, 100}, TEXT("yellow"), false);
    for (float X : {-34.f, 34.f})
        Shape(Coat, Sphere, {X, 0, 17}, {18, 18, 66}, TEXT("yellow"), false)
            ->SetRelativeRotation({0, 0, X > 0 ? 18.f : -18.f});
    Shape(Coat, Sphere, {0, 0, 58}, {30, 23, 30}, TEXT("yellow"), false);
    Box(Coat, {0, -10, 0}, {2, 1, 80}, TEXT("brass"), false);
    Coat->Visible(false);
    auto *Sofa = Group(TEXT("living.sofa"), {785, 239, 0}, 90);
    Box(Sofa, {0, 0, 30}, {300, 100, 55}, TEXT("brown"));
    Box(Sofa, {0, 42, 75}, {300, 22, 90}, TEXT("brown"));
    for (float X : {-140.f, 140.f})
        Box(Sofa, {X, 0, 65}, {25, 100, 55}, TEXT("brown"));
    for (float X : {-90.f, 0.f, 90.f})
        Box(Sofa, {X, -8, 61}, {86, 74, 14}, TEXT("brown"));
    auto *Cat = Group(TEXT("living.cat"), {800, 264, 79}, 90);
    Shape(Cat, Sphere, {0, 0, 0}, {48, 28, 20}, TEXT("black"), false);
    Shape(Cat, Sphere, {-22, 0, 5}, {23, 23, 22}, TEXT("black"), false);
    for (float Y : {-8.f, 8.f})
        Shape(Cat, Cone, {-23, Y, 18}, {11, 10, 15}, TEXT("black"), false);
    // Closely spaced tapered segments form a continuous curled tail.
    for (int I = 0; I < 32; ++I)
    {
        float A = I * .075f;
        float Width = 5.f - I * .07f;
        Shape(Cat, Sphere, {22 + 18 * FMath::Sin(A), -18 + 18 * FMath::Cos(A), -5},
              {Width, Width, Width}, TEXT("black"), false);
    }
    for (float Y : {-6.f, 6.f})
        Box(Cat, {-33, Y, 6}, {1, 5, 1}, TEXT("curtain"), false);
    Cat->Visible(false);
    auto *TV = Group(TEXT("living.tv"), {1102, 235, 155}, 180);
    Box(TV, {0, 0, 0}, {14, 180, 105}, TEXT("black"));
    Box(TV, {9, 0, 0}, {3, 165, 90}, TEXT("night"), false);
    auto *Static = Group(TEXT("living.static"), {1090, 235, 155}, 180);
    for (int X = 0; X < 18; ++X)
        for (int Y = 0; Y < 10; ++Y)
            StaticPixels.Add(Box(Static, {0, -80 + X * 9.f, -40 + Y * 9.f}, {1, 9, 9},
                                 FString::Printf(TEXT("static%d"), (X + Y) % 4), false));
    Label(Static, TEXT("wake up"), {3, 0, 0}, 22);
    Static->Visible(false);
    // Island with an open basin, a faucet and removable dishes.
    auto *Island = Group(TEXT("kitchen.island"), {870, -275, 0}, 90);
    Island->SetActorRelativeScale3D({.66, 1, 1});
    Box(Island, {0, 0, 38}, {160, 200, 76}, TEXT("wood"));
    Box(Island, {-58, 0, 94}, {64, 220, 8}, TEXT("white"));
    Box(Island, {58, 0, 94}, {64, 220, 8}, TEXT("white"));
    Box(Island, {0, -77, 94}, {52, 66, 8}, TEXT("white"));
    Box(Island, {0, 77, 94}, {52, 66, 8}, TEXT("white"));
    Box(Island, {0, 0, 80}, {52, 88, 3}, TEXT("steel"), false);
    for (float X : {-26.f, 26.f})
        Box(Island, {X, 0, 88}, {3, 88, 16}, TEXT("steel"), false);
    for (float Y : {-44.f, 44.f})
        Box(Island, {0, Y, 88}, {52, 3, 16}, TEXT("steel"), false);
    Shape(Island, Cylinder, {0, 50, 110}, {5, 5, 32}, TEXT("steel"), false);
    Box(Island, {0, 36, 125}, {5, 29, 5}, TEXT("steel"), false);
    Shape(Island, Cylinder, {0, 22, 121}, {6, 6, 8}, TEXT("steel"), false);
    Box(Island, {13, 51, 104}, {18, 5, 5}, TEXT("steel"), false);
    auto *Water = Group(TEXT("kitchen.water"), {848, -275, 101});
    Shape(Water, Cylinder, {0, 0, 0}, {2.7f, 2.7f, 37}, TEXT("water"), false);
    Water->Visible(false);
    auto *Dishes = Group(TEXT("kitchen.dishes"), {880, -275, 85});
    for (int I = 0; I < 4; ++I)
    {
        Shape(Dishes, Cylinder, {0, 0, I * 2.f}, {40, 40, 2}, TEXT("white"), false);
        Shape(Dishes, Cylinder, {0, 0, I * 2.f + 1}, {30, 30, .3f}, TEXT("tile2"), false);
    }
    Shape(Dishes, Cylinder, {14, 27, 7}, {13, 13, 15}, TEXT("white"), false);
    Table(TEXT("kitchen.table"), {1335, -275, 0}, {126, 110, 0});
    for (float Y : {-353.f, -197.f})
    {
        auto *Chair = Group(FString::Printf(TEXT("chair.%d"), int(Y)), {1335, Y, 0});
        Box(Chair, {0, 0, 45}, {50, 50, 8}, TEXT("wood"));
        Box(Chair, {0, Y < -275 ? -23.f : 23.f, 74}, {50, 5, 65}, TEXT("wood"));
        for (float X : {-19.f, 19.f})
            for (float S : {-19.f, 19.f})
                Box(Chair, {X, S, 21}, {5, 5, 42}, TEXT("wood"));
    }
    auto *Fridge = Group(TEXT("kitchen.fridge"), {1048, -421, 0}, -90);
    Box(Fridge, {30, 0, 110}, {20, 110, 220}, TEXT("black"));
    for (float Y : {-53.f, 53.f})
        Box(Fridge, {0, Y, 110}, {70, 5, 220}, TEXT("black"));
    for (float Z : {5.f, 217.f})
        Box(Fridge, {0, 0, Z}, {70, 110, 6}, TEXT("black"));
    for (float Z : {55.f, 110.f, 165.f})
        Box(Fridge, {0, 0, Z}, {65, 100, 3}, TEXT("steel"), false);
    auto *FridgeDoor = Group(TEXT("kitchen.fridgeDoor"), {995, -380, 0}, 90);
    Box(FridgeDoor, {0, -53, 110}, {8, 110, 220}, TEXT("black"));
    Box(FridgeDoor, {9, -92, 130}, {9, 5, 45}, TEXT("steel"), false);
    auto *Magnets = Group(TEXT("kitchen.magnets"));
    Magnets->AttachToActor(FridgeDoor, FAttachmentTransformRules::KeepRelativeTransform);
    Magnets->SetActorRelativeTransform(FTransform::Identity);
    const TCHAR *Colors[] = {TEXT("red"), TEXT("yellow"), TEXT("blue")};
    for (int I = 0; I < 6; ++I)
    {
        float Y = -25 - (I % 3) * 19.f, Z = 162 - (I / 3) * 24.f;
        Shape(Magnets, Sphere, {7, Y, Z}, {4, 12, 12}, Colors[I % 3], false);
    }
    Magnets->Visible(false);
}
void ASAHouse::Reset()
{
    ClearStickers();
    for (auto &Door : Doors)
        Door.Value = {};
    SetSleepScene(false, false, 0);
    for (auto P : Groups)
        P->Reset();
    SetCountdown(10);
    TVAge = 0;
    StaticAge = 0;
    if (auto *P = Props.Find(TEXT("living.static")))
        if ((*P)->Words)
            (*P)->Words->SetVisibility(false);
}
void ASAHouse::SetCountdown(int Remaining)
{
    for (auto C : Counters)
        C->SetText(FText::AsNumber(Remaining));
}
void ASAHouse::Animate(float Delta)
{
    auto *TV = Props.FindChecked(TEXT("living.static")).Get();
    if (!TV->IsHidden())
    {
        TVAge += Delta;
        StaticAge += Delta;
        if (StaticAge >= .12f)
        {
            StaticAge = 0;
            for (auto P : StaticPixels)
                P->SetMaterial(0,
                               Materials.FindChecked(FString::Printf(TEXT("static%d"), StaticRandom.RandRange(0, 3))));
        }
        if (TV->Words)
            TV->Words->SetVisibility(FMath::Fmod(TVAge, 5.f) > 3.6f);
    }
    auto *Water = Props.FindChecked(TEXT("kitchen.water")).Get();
    if (!Water->IsHidden())
    {
        float W = 1 + .12f * FMath::Sin(GetWorld()->GetTimeSeconds() * 19);
        Water->SetActorRelativeScale3D({W, W, 1});
    }
}
