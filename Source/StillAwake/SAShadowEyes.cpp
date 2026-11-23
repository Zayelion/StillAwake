#include "SAShadowEyes.h"

#include "SAHouse.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"

namespace
{
constexpr double LookThreshold = 10.0;
constexpr float LightCheckInterval = .12f;
constexpr float EyeSpacing = 9.f;
const FVector EyeScale[] = {{.018, .125, .048}, {.011, .048, .046}, {.009, .023, .026}};
constexpr FLinearColor EyeTints[] = {{.43f, .39f, .26f}, {.13f, .15f, .09f}, {.008f, .01f, .006f}};

bool IsInsideHouse(const FVector &Point)
{
    if (Point.X < 0 || Point.X > 2510 || Point.Z < -1 || Point.Z > 620)
        return false;
    if (Point.X <= 1122)
        return FMath::Abs(Point.Y) <= 470;
    if (Point.Z >= 342 && Point.X <= 1514)
        return Point.Y >= -470 && Point.Y <= 94;
    if (Point.X <= 1554)
        return Point.Y >= -470 && Point.Y <= -35;
    if (Point.X <= 1860)
        return Point.Y >= -630 && Point.Y <= -35;
    return Point.Y >= -795 && Point.Y <= -35;
}

bool CanAnchorTo(const FHitResult &Hit)
{
    const auto *Actor = Hit.GetActor();
    const auto *Component = Hit.GetComponent();
    if (!Actor || !Component || Actor->IsHidden() || Component->bHiddenInGame || !Component->IsVisible())
        return false;
    for (const FName Tag : Actor->Tags)
    {
        const FString Id = Tag.ToString();
        if (Id == TEXT("house.shell") || Id == TEXT("upstairs.structure") || Id.StartsWith(TEXT("floor.")) ||
            Id.EndsWith(TEXT(".door")) || Id == TEXT("upstairs.master.wardrobe") ||
            Id == TEXT("upstairs.master.wardrobe.leaf"))
            return true;
    }
    return false;
}
}

ASAShadowEyes::ASAShadowEyes()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ShadowEyeAnchor"));
    for (float Side : {-1.f, 1.f})
    {
        for (int Layer = 0; Layer < UE_ARRAY_COUNT(EyeScale); ++Layer)
        {
            const FName Name(*FString::Printf(TEXT("EyePart%d"), EyeParts.Num()));
            auto *Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
            Part->SetupAttachment(RootComponent);
            Part->SetMobility(EComponentMobility::Movable);
            Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Part->SetGenerateOverlapEvents(false);
            Part->SetCastShadow(false);
            Part->SetRelativeLocation({.8 + Layer * .65, Side * EyeSpacing, 0});
            EyeParts.Add(Part);
        }
    }
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}

void ASAShadowEyes::Reset()
{
    SurfaceComponent.Reset();
    LookSeconds = 0;
    TimeUntilLightCheck = 0;
    bShadowed = false;
    if (!IsHidden())
        SetActorHiddenInGame(true);
}

bool ASAShadowEyes::IsShadowed(const ASAHouse &House, const FHitResult &Hit, const AActor *Viewer) const
{
    const FVector Probe = Hit.ImpactPoint + Hit.ImpactNormal * 2.;
    const auto Contribution = [&](const UPointLightComponent *Light) -> double
    {
        if (!Light || !Light->IsVisible() || Light->Intensity <= 0)
            return 0;
        const FVector ToLight = Light->GetComponentLocation() - Probe;
        const double DistanceSquared = ToLight.SizeSquared();
        const double RadiusSquared = FMath::Square(double(Light->AttenuationRadius));
        if (DistanceSquared >= RadiusSquared)
            return 0;
        const double Angle = FVector::DotProduct(Hit.ImpactNormal, ToLight.GetSafeNormal());
        if (Angle <= 0)
            return 0;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(SAShadowLight), false, Viewer);
        Query.AddIgnoredActor(Light->GetOwner());
        if (GetWorld()->LineTraceTestByChannel(Probe, Light->GetComponentLocation(), ECC_Visibility, Query))
            return 0;

        // Fixtures use lumens. Approximate direct illuminance with inverse-square falloff and the surface angle.
        const double Falloff = FMath::Square(1. - FMath::Square(DistanceSquared / RadiusSquared));
        return (Light->Intensity * Angle * Falloff) / (4 * PI * FMath::Max(.04, DistanceSquared / 10000.));
    };

    constexpr double ShadowLux = 1.5;
    double Lux = 0;
    for (const auto &Light : House.Lights)
    {
        Lux += Contribution(Light.Get());
        if (Lux >= ShadowLux)
            return false;
    }
    for (const TCHAR *Id : {TEXT("sleep.bedsideLight"), TEXT("sleep.fill")})
        if (const auto *Fixture = House.Props.FindRef(Id).Get())
            Lux += Contribution(Fixture->FindComponentByClass<UPointLightComponent>());
    return Lux < ShadowLux;
}

void ASAShadowEyes::Observe(const ASAHouse *House, const FVector &Origin, const FVector &Direction,
                           const AActor *Viewer, float Delta)
{
    FHitResult Hit;
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(SAShadowGaze), false, Viewer);
    if (!House || !IsInsideHouse(Origin) ||
        !GetWorld()->LineTraceSingleByChannel(Hit, Origin, Origin + Direction * 740, ECC_Visibility, Query) ||
        !IsInsideHouse(Hit.ImpactPoint) || !CanAnchorTo(Hit))
    {
        Reset();
        return;
    }

    if (SurfaceComponent.Get() != Hit.GetComponent() || FVector::DistSquared(FocusPoint, Hit.ImpactPoint) > 36. * 36. ||
        FVector::DotProduct(FocusNormal, Hit.ImpactNormal) < .9)
    {
        Reset();
        SurfaceComponent = Hit.GetComponent();
        FocusPoint = Hit.ImpactPoint;
        FocusNormal = Hit.ImpactNormal;
    }

    TimeUntilLightCheck -= Delta;
    if (TimeUntilLightCheck <= 0)
    {
        bShadowed = IsShadowed(*House, Hit, Viewer);
        TimeUntilLightCheck = LightCheckInterval;
    }
    if (!bShadowed)
    {
        LookSeconds = 0;
        if (!IsHidden())
            SetActorHiddenInGame(true);
        return;
    }
    LookSeconds += Delta;
    if (LookSeconds <= LookThreshold)
        return;

    if (IsHidden())
    {
        PrepareEyes(*House);
        FVector Up = FVector::UpVector;
        if (FMath::Abs(FocusNormal.Z) > .9)
        {
            const FVector FloorDirection = (FocusPoint - Origin).GetSafeNormal2D();
            Up = FloorDirection.IsNearlyZero() ? FVector::ForwardVector : FloorDirection;
        }
        SetActorLocationAndRotation(FocusPoint + FocusNormal * 1.4, FRotationMatrix::MakeFromXZ(FocusNormal, Up).ToQuat());
        SetActorHiddenInGame(false);
    }
    AnimateEyes();
}

void ASAShadowEyes::PrepareEyes(const ASAHouse &House)
{
    if (EyeParts[0]->GetStaticMesh())
        return;
    for (int Layer = 0; Layer < UE_ARRAY_COUNT(EyeScale); ++Layer)
    {
        auto *Material = UMaterialInstanceDynamic::Create(House.Surface, this);
        Material->SetVectorParameterValue(TEXT("Tint"), EyeTints[Layer]);
        Material->SetScalarParameterValue(TEXT("Emission"), .85f);
        Material->SetScalarParameterValue(TEXT("SurfaceDetail"), 0);
        Material->SetScalarParameterValue(TEXT("Roughness"), .83f);
        for (int Side = 0; Side < 2; ++Side)
        {
            auto *Part = EyeParts[Side * UE_ARRAY_COUNT(EyeScale) + Layer].Get();
            Part->SetStaticMesh(House.Sphere);
            Part->SetMaterial(0, Material);
        }
    }
}

void ASAShadowEyes::AnimateEyes()
{
    const double Age = LookSeconds - LookThreshold;
    const double Opening = FMath::SmoothStep(0., .35, Age);
    const double BlinkTime = FMath::Fmod(Age, 2.7);
    const double Blink = FMath::Sin(PI * FMath::Clamp((BlinkTime - 2.5) / .2, 0., 1.));
    const double Aperture = FMath::Max(.04, Opening * (1. - .98 * Blink));
    for (int Index = 0; Index < EyeParts.Num(); ++Index)
    {
        FVector Scale = EyeScale[Index % UE_ARRAY_COUNT(EyeScale)];
        Scale.Z *= Aperture;
        EyeParts[Index]->SetRelativeScale3D(Scale);
    }
}
