#include "SAHouse.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

namespace
{
// The pivot sits inside the clear opening so the heel clears the jamb in either direction.
constexpr double HingeInset = 3.6;
constexpr double LeafLength = 96. - HingeInset - 1.;
constexpr double SwingDuration = 1.65;
constexpr double PushReach = 82.;

double GetSwingAngle(double Progress)
{
    return FMath::Sign(Progress) * 90. * FMath::SmoothStep(0., 1., FMath::Abs(Progress));
}

bool IsPushingDoor(const FVector &Position, const FVector &Direction, double &Crossing)
{
    if (Position.Z < 0 || Position.Z > 208 || Position.X * Direction.X >= 0 || FMath::Abs(Direction.X) < .35)
        return false;
    const double Distance = -Position.X / Direction.X;
    Crossing = Position.Y + Distance * Direction.Y;
    return Distance <= PushReach && Crossing > 0 && Crossing < LeafLength;
}

struct FSwingBox
{
    FVector Center, Extent;
};

// Cover the slab, raised panels and handles of the room door.
const FSwingBox DoorSwingBoxes[] = {
    {{0, LeafLength / 2, 104}, {2, LeafLength / 2, 103}},
    {{0, 48, 55}, {3.6, 36.5, 40}},
    {{0, 48, 151}, {3.6, 36.5, 40}},
    {{0, 83, 103}, {8.8, 5, 7}},
};

bool CanSwingThrough(const UWorld &World, const FTransform &Closed, double From, double To,
                     const FCollisionQueryParams &Query)
{
    const FQuat Rotation = Closed.GetRotation() * FQuat(FVector::UpVector, FMath::DegreesToRadians((From + To) * .5));
    const double HalfArc = FMath::DegreesToRadians(FMath::Abs(To - From) * .5);
    const double Sine = FMath::Sin(HalfArc);
    const double Inset = 1. - FMath::Cos(HalfArc);
    for (const FSwingBox &Part : DoorSwingBoxes)
    {
        // A rotational sweep needs to cover the arc between poses, including during a long frame.
        const double Radius = Part.Center.Y + Part.Extent.Y;
        const FVector Padding(Radius * Sine, Part.Extent.X * Sine + Radius * Inset, 0);
        const FVector Center = Closed.GetLocation() + Rotation.RotateVector(Part.Center);
        if (World.OverlapBlockingTestByProfile(Center, Rotation, TEXT("BlockAll"),
                                               FCollisionShape::MakeBox(Part.Extent + Padding), Query))
            return false;
    }
    return true;
}

double AdvanceSwing(const UWorld &World, const FTransform &Closed, double From, double To,
                    const FCollisionQueryParams &Query)
{
    const double FirstAngle = GetSwingAngle(From), LastAngle = GetSwingAngle(To);
    if (CanSwingThrough(World, Closed, FirstAngle, LastAngle, Query))
        return To;
    // The arc envelope can brush the previous contact while the actual leaf is moving away.
    // Refine just that arc, down to less than 0.01 cm of travel at the latch.
    if (FMath::Abs(LastAngle - FirstAngle) < .005)
        return CanSwingThrough(World, Closed, LastAngle, LastAngle, Query) ? To : From;
    const double Mid = (From + To) * .5;
    const double Clear = AdvanceSwing(World, Closed, From, Mid, Query);
    if (Clear != Mid)
        return Clear;
    return AdvanceSwing(World, Closed, Mid, To, Query);
}
}

void ASAHouse::AddRoomDoor(const FString &Id, FVector Hinge, float Yaw)
{
    const FVector Pivot = Hinge + FRotator(0, Yaw, 0).RotateVector({0, HingeInset, 0});
    auto *Leaf = Group(Id, Pivot, Yaw);
    Doors.Add(Id);
    Box(Leaf, {0, LeafLength / 2, DoorHeight / 2}, {4, LeafLength, DoorHeight - 2}, TEXT("oak"));
    for (float Side : {-1.f, 1.f})
    {
        for (float Z : {55.f, 151.f})
        {
            Box(Leaf, {Side * 2.4f, 48, Z}, {1.1f, 73, 80}, TEXT("walnut"), false);
            Box(Leaf, {Side * 3.2f, 48, Z}, {.8f, 67, 74}, TEXT("doorInset"), false);
        }
        Box(Leaf, {Side * 2.8f, 85, 101}, {2, 6, 10}, TEXT("brass"), false);
        Shape(Leaf, Sphere, {Side * 5.6f, 85, 103}, {6, 5, 5}, TEXT("brass"), false);
        Box(Leaf, {Side * 7.5f, 81, 103}, {2, 10, 2}, TEXT("brass"), false);
    }
    auto *Frame = Group(Id + TEXT(".frame"), Hinge, Yaw);
    for (float Y : {-2.f, 98.f})
    {
        Box(Frame, {0, Y, 106}, {23, 5, 212}, TEXT("white"), false);
        for (float X : {-10.f, 10.f})
            Box(Frame, {X, Y, 106}, {2, 9, 212}, TEXT("white"), false);
    }
    Box(Frame, {0, 48, 211}, {23, 111, 6}, TEXT("white"), false);
    Box(Frame, {0, 48, .85f}, {21, 98, 1.7f}, TEXT("oak"), false);
    for (float Z : {31.f, 105.f, 180.f})
        Shape(Leaf, Cylinder, {0, .8f, Z}, {4, 4, 7}, TEXT("brass"), false);
}

bool ASAHouse::DoorOpen(const FString &Id) const
{
    const auto *Swing = Doors.Find(Id);
    return Swing && FMath::Abs(GetSwingAngle(Swing->Progress)) > 45.;
}

bool ASAHouse::ShouldCloseDoor(const FString &Id) const
{
    const auto *Swing = Doors.Find(Id);
    return Swing && Swing->Target != 0;
}

void ASAHouse::ToggleDoor(const FString &Id, const FVector &Pusher)
{
    auto *Swing = Doors.Find(Id);
    if (!Swing)
        return;
    const FTransform Closed = Props.FindChecked(Id)->Baseline * GetActorTransform();
    const double Side = Closed.InverseTransformPosition(Pusher).X;
    const double OpenSide = Swing->Progress != 0 ? FMath::Sign(Swing->Progress) : (Side >= 0 ? 1. : -1.);
    Swing->Target = ShouldCloseDoor(Id) ? 0. : OpenSide;
    Swing->bBlocked = false;
}

bool ASAHouse::UpdateDoors(float DeltaSeconds, const AActor *Player, const FVector &Travel)
{
    if (DeltaSeconds <= 0)
        return false;
    bool bPushed = false;
    const FVector Direction = Travel.GetSafeNormal2D();
    for (auto &Entry : Doors)
    {
        auto &Swing = Entry.Value;
        auto *Leaf = Props.FindChecked(Entry.Key).Get();
        const FTransform Closed = Leaf->Baseline * GetActorTransform();
        FCollisionQueryParams Query(SCENE_QUERY_STAT(RoomDoor), false, Player);
        if (Player)
        {
            const FVector Origin = Player->GetActorLocation();
            const FVector Local = Closed.InverseTransformPosition(Origin);
            double Crossing = 0;
            bool bApproaching = IsPushingDoor(Local, Closed.InverseTransformVectorNoScale(Direction), Crossing);
            if (bApproaching && !Swing.bApproaching && (Swing.Target == 0 || Swing.bBlocked))
            {
                FHitResult Hit;
                const FVector Contact = Closed.TransformPosition({0, Crossing, Local.Z});
                bApproaching = !GetWorld()->LineTraceSingleByChannel(Hit, Origin, Contact, ECC_Visibility, Query) ||
                               Hit.GetActor() == Leaf;
                if (bApproaching)
                {
                    Swing.Target = Local.X >= 0 ? 1. : -1.;
                    Swing.bBlocked = false;
                    bPushed = true;
                }
            }
            Swing.bApproaching = bApproaching;
        }
        if (Swing.bBlocked || Swing.Progress == Swing.Target)
            continue;
        Query.AddIgnoredActor(Leaf);
        const double End = FMath::FInterpConstantTo(Swing.Progress, Swing.Target, double(DeltaSeconds), 1. / SwingDuration);
        // Smoothstep's steepest slope is 135 degrees per unit of progress.
        const int Steps = FMath::CeilToInt(FMath::Abs(End - Swing.Progress) * 135.);
        const double Start = Swing.Progress;
        for (int Step = 1; Step <= Steps; ++Step)
        {
            const double Next = Step == Steps ? End : FMath::Lerp(Start, End, double(Step) / Steps);
            Swing.Progress = AdvanceSwing(*GetWorld(), Closed, Swing.Progress, Next, Query);
            if (Swing.Progress == Next)
                continue;
            Swing.bBlocked = true;
            break;
        }
        Leaf->SetActorRelativeRotation(Leaf->Baseline.GetRotation() *
                                       FQuat(FVector::UpVector, FMath::DegreesToRadians(GetSwingAngle(Swing.Progress))));
    }
    return bPushed;
}
