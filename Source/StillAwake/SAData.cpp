#include "SAData.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
static float Bounded(float V, float Min, float Max, float Default)
{
    return FMath::IsFinite(V) ? FMath::Clamp(V, Min, Max) : Default;
}
void FSASettings::Validate()
{
    Master = Bounded(Master, 0, 1, .8f);
    Ambience = Bounded(Ambience, 0, 1, .65f);
    SFX = Bounded(SFX, 0, 1, .7f);
    Music = Bounded(Music, 0, 1, .35f);
    UI = Bounded(UI, 0, 1, .6f);
    Sensitivity = Bounded(Sensitivity, .2f, 4, 1.4f);
    FOV = Bounded(FOV, 70, 95, 80);
    Brightness = Bounded(Brightness, .6f, 1.7f, 1);
    SubtitleScale = Bounded(SubtitleScale, .8f, 1.6f, 1);
}

static bool ReadPositiveNumber(const FJsonObject &Object, const TCHAR *Field, double &Value)
{
    return Object.TryGetNumberField(Field, Value) && FMath::IsFinite(Value) && Value > 0;
}

bool FSAContent::LoadCycles(const FJsonObject &Root, FString &Error)
{
    const TArray<TSharedPtr<FJsonValue>> *Items;
    if (!Root.TryGetArrayField(TEXT("inspectableTargets"), Items) || Items->IsEmpty())
    {
        Error = TEXT("Inspectable targets are missing.");
        return false;
    }
    TSet<FString> Targets;
    for (const auto &Item : *Items)
    {
        FString Id;
        if (!Item->TryGetString(Id) || Id.IsEmpty() || Targets.Contains(Id))
        {
            Error = TEXT("Invalid or duplicate inspectable target: ") + Id;
            return false;
        }
        Targets.Add(Id);
        InspectableTargets.Add(MoveTemp(Id));
    }

    if (!Root.TryGetArrayField(TEXT("hunts"), Items) || Items->Num() != SA::CycleCount)
    {
        Error = TEXT("Exactly three hunts must be authored.");
        return false;
    }
    double MaxSleepPressure = 0;
    for (const auto &Item : *Items)
    {
        const auto Object = Item->AsObject();
        const TArray<TSharedPtr<FJsonValue>> *AnomalyIds;
        FSAHunt Hunt;
        if (!Object || !Object->TryGetArrayField(TEXT("anomalyIds"), AnomalyIds) || AnomalyIds->IsEmpty() ||
            !Object->TryGetStringField(TEXT("startLine"), Hunt.StartLine) || Hunt.StartLine.IsEmpty() ||
            !Object->TryGetStringField(TEXT("sleepLine"), Hunt.SleepLine) || Hunt.SleepLine.IsEmpty() ||
            !ReadPositiveNumber(*Object, TEXT("sleepPressure"), Hunt.SleepPressure))
        {
            Error = TEXT("Invalid hunt record.");
            return false;
        }
        MaxSleepPressure = FMath::Max(MaxSleepPressure, Hunt.SleepPressure);
        TSet<FString> ActiveTargets;
        for (const auto &Value : *AnomalyIds)
        {
            FString Id;
            if (!Value->TryGetString(Id) || Id.IsEmpty())
            {
                Error = TEXT("Invalid anomaly ID in hunt.");
                return false;
            }
            const int Index = Anomalies.IndexOfByPredicate([&](const FSAAnomaly &Anomaly) { return Anomaly.Id == Id; });
            if (Index == INDEX_NONE || !Targets.Contains(Anomalies[Index].Target) ||
                ActiveTargets.Contains(Anomalies[Index].Target))
            {
                Error = TEXT("Unknown, uninspectable, or conflicting hunt anomaly: ") + Id;
                return false;
            }
            ActiveTargets.Add(Anomalies[Index].Target);
            Hunt.AnomalyIndices.Add(Index);
        }
        Hunts.Add(MoveTemp(Hunt));
    }

    constexpr const TCHAR *Profiles[] = {TEXT("gentle"), TEXT("manageable"), TEXT("pressured"), TEXT("severe"),
                                         TEXT("extreme")};
    if (!Root.TryGetArrayField(TEXT("sleepProfiles"), Items) || Items->Num() != UE_ARRAY_COUNT(Profiles))
    {
        Error = TEXT("Exactly five sleep profiles must be authored.");
        return false;
    }
    for (int Index = 0; Index < Items->Num(); ++Index)
    {
        const auto Object = (*Items)[Index]->AsObject();
        FString Name;
        SA::SleepProfile Profile;
        if (!Object || !Object->TryGetStringField(TEXT("name"), Name) || Name != Profiles[Index] ||
            !ReadPositiveNumber(*Object, TEXT("anxietyRiseRate"), Profile.AnxietyRiseRate) ||
            !ReadPositiveNumber(*Object, TEXT("anxietyRecoveryRate"), Profile.AnxietyRecoveryRate) ||
            !ReadPositiveNumber(*Object, TEXT("closetAdvanceRate"), Profile.ClosetAdvanceRate) ||
            !ReadPositiveNumber(*Object, TEXT("closetRetreatRate"), Profile.ClosetRetreatRate) ||
            !ReadPositiveNumber(*Object, TEXT("anxietyFailureThreshold"), Profile.AnxietyFailureThreshold) ||
            !ReadPositiveNumber(*Object, TEXT("closetFailureThreshold"), Profile.ClosetFailureThreshold) ||
            !ReadPositiveNumber(*Object, TEXT("warningSeconds"), Profile.WarningSeconds) ||
            !ReadPositiveNumber(*Object, TEXT("sleepDuration"), Profile.SleepDuration) ||
            Profile.WarningSeconds * Profile.ClosetAdvanceRate * MaxSleepPressure >= Profile.ClosetFailureThreshold)
        {
            Error = FString(TEXT("Invalid sleep profile: ")) + Profiles[Index];
            return false;
        }
        SleepProfiles.Add(Profile);
    }
    if (!Root.TryGetStringField(TEXT("endingLine"), EndingLine) || EndingLine.IsEmpty())
    {
        Error = TEXT("The ending line is missing.");
        return false;
    }
    return true;
}

bool FSAContent::Load(FString &Error)
{
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Data/content.json"))))
    {
        Error = TEXT("The authored content file is missing.");
        return false;
    }
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root)
    {
        Error = TEXT("The authored content file is invalid JSON.");
        return false;
    }
    Anomalies.Reset();
    Stories.Reset();
    Hunts.Reset();
    InspectableTargets.Reset();
    SleepProfiles.Reset();
    const TArray<TSharedPtr<FJsonValue>> *Items;
    if (!Root->TryGetArrayField(TEXT("anomalies"), Items))
    {
        Error = TEXT("The anomaly library is missing.");
        return false;
    }
    TSet<FString> Ids;
    for (const auto &Item : *Items)
    {
        const auto O = Item->AsObject();
        if (!O)
        {
            Error = TEXT("Invalid anomaly record.");
            return false;
        }
        FSAAnomaly A;
        double Room, Tier, Weight, Effect, Enabled, Progress;
        if (!O->TryGetStringField(TEXT("anomalyId"), A.Id) || !O->TryGetStringField(TEXT("displayName"), A.Name) ||
            !O->TryGetStringField(TEXT("targetId"), A.Target) || !O->TryGetNumberField(TEXT("routeIndex"), Room) ||
            !O->TryGetNumberField(TEXT("tier"), Tier) || !O->TryGetNumberField(TEXT("selectionWeight"), Weight) ||
            !O->TryGetNumberField(TEXT("effect"), Effect) || !O->TryGetNumberField(TEXT("enabledState"), Enabled) ||
            !O->TryGetNumberField(TEXT("minProgress"), Progress))
        {
            Error = TEXT("An anomaly record is incomplete.");
            return false;
        }
        if (Ids.Contains(A.Id) || Room < 1 || Room > 6 || Tier < 1 || Tier > 3 || Weight <= 0 || Effect < 0 ||
            Effect > 6)
        {
            Error = TEXT("Invalid or duplicate anomaly: ") + A.Id;
            return false;
        }
        Ids.Add(A.Id);
        A.Room = Room;
        A.Tier = Tier;
        A.Weight = Weight;
        A.Effect = Effect;
        A.Enabled = Enabled != 0;
        A.MinProgress = Progress;
        O->TryGetStringField(TEXT("replacementText"), A.Text);
        const TArray<TSharedPtr<FJsonValue>> *Values;
        if (!O->TryGetArrayField(TEXT("value"), Values) || Values->Num() != 3)
        {
            Error = TEXT("Invalid effect vector.");
            return false;
        }
        A.Value = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
        Anomalies.Add(A);
    }
    Anomalies.Sort([](const auto &A, const auto &B) { return A.Id < B.Id; });
    if (!Root->TryGetArrayField(TEXT("story"), Items))
    {
        Error = TEXT("The story library is missing.");
        return false;
    }
    for (const auto &Item : *Items)
    {
        auto O = Item->AsObject();
        FSAStory S;
        double Unlock;
        if (!O || !O->TryGetStringField(TEXT("fragmentId"), S.Id) || !O->TryGetStringField(TEXT("title"), S.Title) ||
            !O->TryGetStringField(TEXT("text"), S.Text) || !O->TryGetNumberField(TEXT("unlockAfterRounds"), Unlock))
        {
            Error = TEXT("Invalid story record.");
            return false;
        }
        S.Unlock = Unlock;
        Stories.Add(S);
    }
    Stories.StableSort([](const auto &A, const auto &B) { return A.Unlock < B.Unlock; });
    double V;
    if (Root->TryGetNumberField(TEXT("walkSpeed"), V))
        WalkSpeed = FMath::Clamp(float(V), 100.f, 400.f);
    if (Anomalies.IsEmpty() || Stories.IsEmpty())
    {
        Error = TEXT("The authored libraries are empty.");
        return false;
    }
    return LoadCycles(*Root, Error);
}
