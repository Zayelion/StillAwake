#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SAHouse.generated.h"
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTextRenderComponent;
class UPointLightComponent;
class UStaticMeshComponent;
UCLASS()
class STILLAWAKE_API ASAProp : public AActor
{
    GENERATED_BODY()
  public:
    ASAProp();
    UPROPERTY() TObjectPtr<UTextRenderComponent> Words;
    UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> BaselineMaterials;
    FTransform Baseline;
    FString BaselineText;
    bool bBaselineVisible = true, bTinted = false;
    void Visible(bool Value);
    void Capture();
    void Reset();
    void Tint(const FLinearColor &Color);
};
UCLASS()
class STILLAWAKE_API ASAHouse : public AActor
{
    GENERATED_BODY()
  public:
    ASAHouse();
    void Build();
    void Reset();
    void ApplyBrightness(float Value, bool Dawn = false);
    void SetCountdown(int Remaining);
    void Animate(float Delta);
    void ToggleDoor(const FString &Id, const FVector &Pusher);
    bool UpdateDoors(float DeltaSeconds, const AActor *Player = nullptr, const FVector &Travel = FVector::ZeroVector);
    bool ShouldCloseDoor(const FString &Id) const;
    bool DoorOpen(const FString &Id) const;
    FString FindInspectable(const FVector &Start, const FVector &Direction, const TSet<FString> &Targets,
                            const AActor *Ignore, FVector *StickerPoint = nullptr) const;
    void AddSticker(const FString &Target, const FVector &Position, const FVector &ViewLocation);
    void ClearStickers();
    void SetSleepScene(bool bActive, bool bLampOn, float ClosetProgress, bool bReducedIntensity = false,
                       float DeltaSeconds = 0.f);
    FVector BedInteractionLocation() const;
    FVector BedViewLocation() const;
    static FRotator BedViewRotation();
    static FVector PorchStart()
    {
        return {-145, 0, 90};
    }
    static FVector MapExitToEntrance(const FVector &Position, bool Back)
    {
        if (Back)
        {
            return FVector(1122 - Position.X, 60 - Position.Y, Position.Z);
        }
        return Position;
    }
    UPROPERTY() TMap<FString, TObjectPtr<ASAProp>> Props;
    UPROPERTY() TArray<TObjectPtr<ASAProp>> Groups;
    UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Lights;
    UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> Counters;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> StaticPixels;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
    UPROPERTY() TObjectPtr<UStaticMesh> Cylinder;
    UPROPERTY() TObjectPtr<UStaticMesh> Cone;
    UPROPERTY() TObjectPtr<UMaterialInterface> Surface;
    UPROPERTY() TMap<FString, TObjectPtr<UMaterialInstanceDynamic>> Materials;

  private:
    struct FDoorSwing
    {
        double Progress = 0, Target = 0;
        bool bApproaching = false, bBlocked = false;
    };
    TMap<FString, FDoorSwing> Doors;
    static constexpr double DoorWidth = 96;
    static constexpr double DoorHeight = 208;
    float TVAge = 0, StaticAge = 0;
    float BrightnessScale = 1.f;
    float ClosetOpacity = 0.f;
    bool bSleepSceneActive = false, bBedsideLampOn = false, bReducedSleepLighting = false;
    UPROPERTY() TArray<TObjectPtr<ASAProp>> Stickers;
    UPROPERTY() TObjectPtr<ASAProp> ClosetLeaf;
    UPROPERTY() TObjectPtr<ASAProp> ClosetPresence;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ClosetPresenceMaterial;
    UPROPERTY() TObjectPtr<UPointLightComponent> BedroomCeilingLight;
    UPROPERTY() TObjectPtr<UPointLightComponent> BedsideLight;
    UPROPERTY() TObjectPtr<UPointLightComponent> SleepFill;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> BedsideShade;
    TArray<float> FixtureLumens;
    FRandomStream StaticRandom{1926};
    ASAProp *Group(const FString &Id, FVector Position = FVector::ZeroVector, float Yaw = 0);
    UStaticMeshComponent *Shape(ASAProp *Parent, UStaticMesh *Mesh, FVector Position, FVector Size,
                                const FString &Material, bool Solid = true);
    UStaticMeshComponent *Box(ASAProp *Parent, FVector Position, FVector Size, const FString &Material,
                              bool Solid = true);
    UTextRenderComponent *Label(ASAProp *Parent, const FString &Text, FVector Position, float Size = 18,
                                FRotator Rotation = FRotator::ZeroRotator);
    void TileFloor(FVector Center, FVector Size, float Yaw = 0);
    void Table(const FString &Id, FVector Position, FVector Size);
    void Window(float X, bool Opening);
    void Painting(float X, bool White);
    void Door(const FString &Id, FVector Hinge, float Yaw);
    void Furnish();
    void BuildGroundFloor();
    void DetailFirstFloor();
    void AddFixture(const FString &Id, FVector Position, float Lumens, float Radius, const FLinearColor &Color);
    void BuildUpstairs();
    void FurnishUpstairs();
    void AddRoomDoor(const FString &Id, FVector Hinge, float Yaw);
    void BuildSleepScene();
    void ApplySleepBrightness();
    ASAProp *FindInspectionBacking(const FString &Target) const;
};
