#include "SAHouse.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
constexpr float InspectionReach = 320.f;
constexpr float InspectionMargin = 9.f;
constexpr float ClosetFadeDuration = .55f;
const FName PresenceOpacityParameter(TEXT("PresenceOpacity"));
}

ASAProp *ASAHouse::FindInspectionBacking(const FString &Target) const
{
    // A screen, switch or support still gives a visible reference when an effect is absent.
    constexpr const TCHAR *Anchors[][2] = {{TEXT("foyer.raincoat"), TEXT("foyer.hooks")},
                                          {TEXT("living.static"), TEXT("living.tv")},
                                          {TEXT("living.cat"), TEXT("living.sofa")},
                                          {TEXT("kitchen.water"), TEXT("kitchen.island")},
                                          {TEXT("kitchen.dishes"), TEXT("kitchen.island")},
                                          {TEXT("upstairs.master.photo"), TEXT("upstairs.master.nightstand")},
                                          {TEXT("kitchen.magnets"), TEXT("kitchen.fridgeDoor")}};
    for (const auto &Anchor : Anchors)
        if (Target == Anchor[0])
            return Props.FindRef(Anchor[1]).Get();
    return Props.FindRef(Target).Get();
}

FString ASAHouse::FindInspectable(const FVector &Start, const FVector &Direction, const TSet<FString> &Targets,
                                  const AActor *Ignore, FVector *StickerPoint) const
{
    const FVector Aim = Direction.GetSafeNormal();
    const FVector End = Start + Aim * InspectionReach;
    FCollisionQueryParams Query;
    if (Ignore)
        Query.AddIgnoredActor(Ignore);

    FString Best;
    FVector BestPoint = FVector::ZeroVector;
    double BestCost = InspectionReach + 1;
    FHitResult Direct;
    if (GetWorld()->LineTraceSingleByChannel(Direct, Start, End, ECC_Visibility, Query))
        if (const auto *Actor = Direct.GetActor())
            for (const FName Tag : Actor->Tags)
            {
                const FString Id = Tag.ToString();
                if (!Targets.Contains(Id))
                    continue;
                Best = Id;
                BestPoint = Direct.ImpactPoint;
                BestCost = Direct.Distance;
                break;
            }

    for (const FString &Id : Targets)
    {
        const ASAProp *Prop = Props.FindRef(Id).Get();
        if (!Prop || Id == Best)
            continue;
        const ASAProp *Backing = FindInspectionBacking(Id);
        if (Prop->IsHidden() && !Prop->bBaselineVisible && (!Backing || Backing == Prop || Backing->IsHidden()))
            continue;

        const FBox Bounds = Prop->GetComponentsBoundingBox(true);
        FVector Near, Normal;
        float HitTime;
        if (!Bounds.IsValid || !FMath::LineExtentBoxIntersection(Bounds, Start, End, FVector(InspectionMargin),
                                                                 Near, Normal, HitTime))
            continue;

        FVector Point = Bounds.GetClosestPointTo(Near);
        const double Distance = FVector::DotProduct(Point - Start, Aim);
        const double Miss = FVector::Dist(Point, Start + Aim * Distance);
        const double Cost = Distance + 3 * Miss;
        if (Cost > BestCost || (Cost == BestCost && !Best.IsEmpty() && Id.Compare(Best) >= 0) ||
            Distance < 0 || Distance > InspectionReach)
            continue;

        FHitResult Obstruction;
        const FVector ThroughPoint = Point + (Point - Start).GetSafeNormal() * 3;
        if (GetWorld()->LineTraceSingleByChannel(Obstruction, Start, ThroughPoint, ECC_Visibility, Query))
        {
            if (Obstruction.GetActor() != Prop && Obstruction.GetActor() != Backing)
                continue;
            Point = Obstruction.ImpactPoint;
        }
        Best = Id;
        BestCost = Cost;
        BestPoint = Point;
    }

    if (StickerPoint && !Best.IsEmpty())
        *StickerPoint = BestPoint;
    return Best;
}

void ASAHouse::AddSticker(const FString &Target, const FVector &Position, const FVector &ViewLocation)
{
    auto *Prop = Props.FindRef(Target).Get();
    if (!Prop)
        return;
    FVector AttachmentPoint = Position;
    bool bFlat = false;
    if (Prop->IsHidden())
    {
        const FVector MissingItemLocation = Prop->GetActorLocation();
        Prop = FindInspectionBacking(Target);
        if (Prop && (Target == TEXT("kitchen.dishes") || Target == TEXT("upstairs.master.photo")))
        {
            const double SurfaceHeight = Target == TEXT("kitchen.dishes") ? 81.5 : 68.5;
            AttachmentPoint = {MissingItemLocation.X, MissingItemLocation.Y, Prop->GetActorLocation().Z + SurfaceHeight};
            bFlat = true;
        }
    }
    if (!Prop)
        return;

    const FVector Normal = bFlat ? FVector::UpVector : (ViewLocation - AttachmentPoint).GetSafeNormal();
    const FVector Up = (AttachmentPoint - ViewLocation).GetSafeNormal2D();
    const FRotator Rotation = bFlat ? FRotationMatrix::MakeFromXZ(Normal, Up).Rotator() : Normal.Rotation();
    auto *Sticker = GetWorld()->SpawnActor<ASAProp>(AttachmentPoint + (bFlat ? 0.3f : 3.f) * Normal, Rotation);
    Sticker->AttachToActor(Prop, FAttachmentTransformRules::KeepWorldTransform);
    Sticker->Tags.Add(TEXT("FixItSticker"));
    Box(Sticker, FVector::ZeroVector, {0.35f, 16.f, 7.f}, TEXT("stickerPaper"), false)->SetCastShadow(false);
    Label(Sticker, TEXT("FIX IT"), {.23f, 0, 0}, 4.2f)->SetTextRenderColor(FColor(31, 25, 19));
    Stickers.Add(Sticker);
}

void ASAHouse::ClearStickers()
{
    for (const auto &Sticker : Stickers)
        if (IsValid(Sticker.Get()))
            Sticker->Destroy();
    Stickers.Reset();
}

FVector ASAHouse::BedInteractionLocation() const
{
    return Props.FindChecked(TEXT("upstairs.master.bed"))->GetActorTransform().TransformPosition({3, 106, 68});
}

FVector ASAHouse::BedViewLocation() const
{
    return Props.FindChecked(TEXT("upstairs.master.bed"))->GetActorTransform().TransformPosition({-27, -80, 74});
}

FRotator ASAHouse::BedViewRotation()
{
    return {-4, 68, 0};
}

void ASAHouse::BuildSleepScene()
{
    const FVector ClosetPosition(1371, -8, 342);
    auto *Wardrobe = Group(TEXT("upstairs.master.wardrobe"), ClosetPosition);
    Box(Wardrobe, {0, 20, 92}, {70, 4, 184}, TEXT("walnut"));
    for (float X : {-33.f, 33.f})
        Box(Wardrobe, {X, 0, 92}, {4, 43, 184}, TEXT("walnut"));
    Box(Wardrobe, {0, 0, 4}, {70, 43, 8}, TEXT("oak"));
    Box(Wardrobe, {0, 0, 183}, {73, 45, 4}, TEXT("oak"));
    Box(Wardrobe, {0, 17.5f, 91}, {61, .6f, 170}, TEXT("slate"), false);

    ClosetLeaf = Group(TEXT("upstairs.master.wardrobe.leaf"), ClosetPosition + FVector(32, -22, 0));
    ClosetLeaf->AttachToActor(Wardrobe, FAttachmentTransformRules::KeepWorldTransform);
    Box(ClosetLeaf, {-32, 0, 94}, {64, 3, 169}, TEXT("oak"));
    for (float X : {-47.f, -17.f})
    {
        Box(ClosetLeaf, {X, -2, 125}, {25, 1, 86}, TEXT("walnut"), false);
        Box(ClosetLeaf, {X, -2, 50}, {25, 1, 53}, TEXT("walnut"), false);
    }
    Box(ClosetLeaf, {-57, -2.8f, 96}, {5, 3, 14}, TEXT("brass"), false);

    ClosetPresence = Group(TEXT("upstairs.master.wardrobe.presence"), ClosetPosition + FVector(-6, 3, 7));
    ClosetPresence->AttachToActor(Wardrobe, FAttachmentTransformRules::KeepWorldTransform);
    auto *FadeParent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StillAwake/M_ClosetPresence.M_ClosetPresence"));
    if (FadeParent)
    {
        ClosetPresenceMaterial = UMaterialInstanceDynamic::Create(FadeParent, ClosetPresence);
        ClosetPresenceMaterial->SetScalarParameterValue(PresenceOpacityParameter, 0.f);
    }
    auto AddBodyPart = [&](const FVector &Position, const FVector &Size)
    {
        auto *Part = Shape(ClosetPresence, Sphere, Position, Size, TEXT("black"), false);
        if (ClosetPresenceMaterial)
            Part->SetMaterial(0, ClosetPresenceMaterial);
        Part->SetCastShadow(false);
        return Part;
    };
    AddBodyPart({0, 0, 144}, {19, 15, 30});
    AddBodyPart({0, 0, 103}, {38, 20, 66});
    for (float Side : {-1.f, 1.f})
    {
        AddBodyPart({Side * 18, 0, 89}, {9, 13, 59})->SetRelativeRotation({0, 0, Side * 8});
        AddBodyPart({Side * 8, 0, 39}, {16, 15, 78});
    }
    ClosetPresence->Visible(false);

    auto AddLight = [this](const TCHAR *Id, const FVector &Position, const FLinearColor &Color, float Radius)
    {
        auto *Fixture = Group(Id, Position);
        auto *Light = NewObject<UPointLightComponent>(Fixture);
        Fixture->AddInstanceComponent(Light);
        Light->SetupAttachment(Fixture->GetRootComponent());
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensityUnits(ELightUnits::Lumens);
        Light->SetLightColor(Color);
        Light->SetAttenuationRadius(Radius);
        Light->SetSourceRadius(6);
        Light->SetVisibility(false);
        Light->RegisterComponent();
        return Light;
    };
    BedsideLight = AddLight(TEXT("sleep.bedsideLight"), BedsideShade->GetComponentLocation() + FVector(0, 0, 14),
                            {1, .75f, .46f}, 465);
    SleepFill = AddLight(TEXT("sleep.fill"), {1260, -120, 543}, {.55f, .68f, 1}, 400);
    ApplySleepBrightness();
}

void ASAHouse::ApplySleepBrightness()
{
    if (!BedsideLight || !SleepFill)
        return;
    BedsideLight->SetIntensity((bReducedSleepLighting ? 98.f : 150.f) * BrightnessScale);
    SleepFill->SetIntensity((bReducedSleepLighting ? 42.f : 30.f) * BrightnessScale);
}

void ASAHouse::SetSleepScene(bool bActive, bool bLampOn, float ClosetProgress, bool bReducedIntensity, float DeltaSeconds)
{
    if (!ClosetLeaf)
        return;

    const bool bLampLit = bActive && bLampOn;
    if (bSleepSceneActive != bActive || bBedsideLampOn != bLampLit || bReducedSleepLighting != bReducedIntensity)
    {
        if (bActive && !bSleepSceneActive)
        {
            Doors.FindChecked(TEXT("upstairs.master.door")) = {};
            Props.FindChecked(TEXT("upstairs.master.door"))->Reset();
        }
        bSleepSceneActive = bActive;
        bBedsideLampOn = bLampLit;
        bReducedSleepLighting = bReducedIntensity;
        BedroomCeilingLight->SetVisibility(!bActive);
        BedsideLight->SetVisibility(bLampLit);
        SleepFill->SetVisibility(bActive);
        BedsideShade->SetMaterial(0, Materials.FindChecked(bLampLit ? TEXT("lampGlass") : TEXT("linen")));
        ApplySleepBrightness();
    }

    const float Progress = bActive ? FMath::Clamp(ClosetProgress, 0.f, 1.f) : 0.f;
    ClosetLeaf->SetActorRelativeRotation(FRotator(0, 91.f * Progress, 0));
    ClosetPresence->SetActorRelativeLocation(ClosetPresence->Baseline.GetLocation() -
                                             FVector(0, Progress * (bReducedIntensity ? 12.f : 32.f), 0));

    const float Opacity = bLampLit ? FMath::Min(1.f, ClosetOpacity + FMath::Max(0.f, DeltaSeconds) / ClosetFadeDuration)
                                  : 0.f;
    if (Opacity != ClosetOpacity)
    {
        ClosetOpacity = Opacity;
        if (ClosetPresenceMaterial)
            ClosetPresenceMaterial->SetScalarParameterValue(PresenceOpacityParameter, ClosetOpacity);
    }
    const bool bShowPresence = ClosetOpacity > 0.f;
    if (ClosetPresence->IsHidden() == bShowPresence)
        ClosetPresence->SetActorHiddenInGame(!bShowPresence);
}
