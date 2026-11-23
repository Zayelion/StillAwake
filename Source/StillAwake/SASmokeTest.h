#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SASmokeTest.generated.h"
class ASAGameMode;

// Opt-in integration harness: uses real character movement/collision and a separate save slot.
UCLASS()
class STILLAWAKE_API ASASmokeTest : public AActor
{
    GENERATED_BODY()
  public:
    ASASmokeTest();
    virtual void Tick(float Delta) override;

  private:
    struct FWaypoint
    {
        FVector Position;
        const TCHAR *PassedDoor = nullptr;
        const TCHAR *Inspection = nullptr;
        const TCHAR *Image = nullptr;
        FVector View = FVector::ZeroVector;
        bool bUseBed = false;
    };
    UPROPERTY() TObjectPtr<ASAGameMode> Game;
    TArray<FWaypoint> Waypoints;
    int Scenario = -1, Waypoint = 0, Checks = 0;
    float Age = 0, ScenarioAge = 0, Wait = 2, StalledTime = 0;
    float FrontDoorTravelTime = 0;
    double PreviousFrontDoorAngle = 0;
    FVector LastProgressPosition = FVector::ZeroVector;
    const TCHAR *PendingImage = nullptr;
    bool bComplete = false, bEndingCaptured = false, bPassedFrontDoor = false;
    void BeginTraversal();
    void FollowRoute(float Delta);
    void CheckAnomaliesAndStart();
    void TrySleep();
    void VerifyEnding();
    void AimAt(const FVector &Target) const;
    bool ObserveFrontDoor(float Delta);
    bool VerifyDoorPassage(const FString &Id);
    bool PlaceSticker(const FString &Id, const FVector &Target);
    void Finish(bool Success, const FString &Detail);
    void Capture(const TCHAR *Name);
};
