#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/SACycleRules.h"
#include "SAData.generated.h"

class FJsonObject;

USTRUCT()
struct FSASettings
{
    GENERATED_BODY()
    UPROPERTY() float Master = .8f;
    UPROPERTY() float Ambience = .65f;
    UPROPERTY() float SFX = .7f;
    UPROPERTY() float Music = .35f;
    UPROPERTY() float UI = .6f;
    UPROPERTY() float Sensitivity = 1.4f;
    UPROPERTY() float FOV = 80;
    UPROPERTY() float Brightness = 1;
    UPROPERTY() bool InvertY = false;
    UPROPERTY() bool Captions = true;
    UPROPERTY() bool CameraMotion = false;
    UPROPERTY() float SubtitleScale = 1;
    UPROPERTY() bool ReducedIntensity = false;
    void Validate();
};
UCLASS()
class STILLAWAKE_API USASaveGame : public USaveGame
{
    GENERATED_BODY()
  public:
    UPROPERTY() int Version = 2;
    UPROPERTY() FSASettings Settings;
    UPROPERTY() int BestStreak = 0;
    UPROPERTY() int TotalRounds = 0;
    UPROPERTY() int TotalCorrect = 0;
    UPROPERTY() int TutorialStep = 0;
    UPROPERTY() bool EndingSeen = false;
    UPROPERTY() TArray<FString> Unlocked;
};
struct FSAAnomaly
{
    FString Id, Name, Target, Text;
    int Room = 1, Tier = 1, MinProgress = 0, Effect = 0;
    float Weight = 1;
    FVector Value = FVector::ZeroVector;
    bool Enabled = false;
};
struct FSAStory
{
    FString Id, Title, Text;
    int Unlock = 0;
};
struct FSAHunt
{
    TArray<int> AnomalyIndices;
    FString StartLine, SleepLine;
    double SleepPressure = 1;
};
struct FSAContent
{
    TArray<FSAAnomaly> Anomalies;
    TArray<FSAStory> Stories;
    TArray<FSAHunt> Hunts;
    TArray<FString> InspectableTargets;
    TArray<SA::SleepProfile> SleepProfiles;
    FString EndingLine;
    float WalkSpeed = 300;
    bool Load(FString &Error);

  private:
    bool LoadCycles(const FJsonObject &Root, FString &Error);
};
