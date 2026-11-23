#include "Misc/AutomationTest.h"
#include "Core/SARuleChecks.h"
#include "SAData.h"
#include "SAHouse.h"
#include "SABuildInfo.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSAPorchLoopTest, "StillAwake.Traversal.PorchMapping",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSAPorchLoopTest::RunTest(const FString &Parameters)
{
    for (float Offset : {-80.f, 0.f, 80.f})
    {
        const FVector Front(-145, Offset, 90);
        const FVector Back(1122 + 145, 60 - Offset, 90);
        TestTrue(TEXT("Back exit preserves porch position and height"),
                 ASAHouse::MapExitToEntrance(Back, true).Equals(Front));
        TestTrue(TEXT("Front exit preserves position"), ASAHouse::MapExitToEntrance(Front, false).Equals(Front));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSACoreTest, "StillAwake.Rules.Acceptance",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSACoreTest::RunTest(const FString &Parameters)
{
    const auto R = SA::RunChecks();
    TestTrue(TEXT("Acceptance suite is populated"), R.Count >= 35);
    for (const auto &Error : R.Errors)
        AddError(UTF8_TO_TCHAR(Error.c_str()));
    return R.Errors.empty();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSAContentTest, "StillAwake.Content.AuthoredLibrary",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSAContentTest::RunTest(const FString &Parameters)
{
    FSAContent C;
    FString Error;
    if (!C.Load(Error))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Fourteen anomalies"), C.Anomalies.Num(), 14);
    TestEqual(TEXT("Six fragments"), C.Stories.Num(), 6);
    TestEqual(TEXT("Three authored hunts"), C.Hunts.Num(), 3);
    TestEqual(TEXT("Five sleep profiles"), C.SleepProfiles.Num(), 5);
    TestEqual(TEXT("Walking is 50 percent faster"), C.WalkSpeed, 300.f);
    TSet<int> Rooms;
    for (const auto &A : C.Anomalies)
        Rooms.Add(A.Room);
    TestEqual(TEXT("Anomalies span six rooms"), Rooms.Num(), 6);
    TSet<FString> Chosen;
    for (const auto &Hunt : C.Hunts)
    {
        TestEqual(TEXT("Four active anomalies per hunt"), Hunt.AnomalyIndices.Num(), 4);
        for (const int Index : Hunt.AnomalyIndices)
        {
            const auto &Anomaly = C.Anomalies[Index];
            TestTrue(TEXT("Active target can be inspected"), C.InspectableTargets.Contains(Anomaly.Target));
            TestFalse(TEXT("Hunts use distinct anomalies"), Chosen.Contains(Anomaly.Id));
            Chosen.Add(Anomaly.Id);
        }
    }
    TestTrue(TEXT("A hunt uses the hall bathroom towel"), Chosen.Contains(TEXT("ANOM_UPSTAIRS_BATH_TOWEL")));
    TestTrue(TEXT("A hunt uses the child's doll"), Chosen.Contains(TEXT("ANOM_UPSTAIRS_CHILD_DOLL")));
    TestTrue(TEXT("A hunt uses the master bedroom photo"), Chosen.Contains(TEXT("ANOM_UPSTAIRS_MASTER_PHOTO")));
    TestFalse(TEXT("Ending line is authored"), C.EndingLine.IsEmpty());

    for (int Index = 0; Index < C.SleepProfiles.Num(); ++Index)
    {
        auto Profile = C.SleepProfiles[Index];
        Profile.AnxietyRiseRate *= C.Hunts.Last().SleepPressure;
        Profile.ClosetAdvanceRate *= C.Hunts.Last().SleepPressure;
        SA::Sleep Sleep;
        Sleep.Reset(Profile);
        for (int Tick = 0; Tick < 6000 && Sleep.Outcome() == SA::SleepOutcome::InProgress; ++Tick)
        {
            if ((!Sleep.IsLampOn() && Sleep.Closet() >= .75) || (Sleep.IsLampOn() && Sleep.Closet() <= .45))
                Sleep.ToggleLamp();
            Sleep.Advance(.02);
        }
        TestTrue(*FString::Printf(TEXT("Final night can be won with authored profile %d"), Index),
                 Sleep.Outcome() == SA::SleepOutcome::Asleep);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSABuildIdentityTest, "StillAwake.Build.Identity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSABuildIdentityTest::RunTest(const FString &Parameters)
{
    const FString Directory = FPaths::ProjectDir() / TEXT(".tmp/BuildIdentity") / FGuid::NewGuid().ToString();
    if (!TestTrue(TEXT("Create identity fixture directory"), IFileManager::Get().MakeDirectory(*Directory, true)))
        return false;
    FString ProjectVersion;
    TestTrue(TEXT("Project version is configured"),
             GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"),
                               ProjectVersion, GGameIni));
    TestFalse(TEXT("Project version is populated"), ProjectVersion.IsEmpty());
    const auto Local = FSABuildInfo::Load(Directory / TEXT("missing.json"));
    TestEqual(TEXT("Missing stamp uses project version"), Local.Version, ProjectVersion);
    TestEqual(TEXT("Missing stamp has local build number"), Local.BuildNumber, FString(TEXT("local")));
    TestEqual(TEXT("Missing stamp has base build version"), Local.BuildVersion, ProjectVersion);
    TestEqual(TEXT("Missing stamp has local run ID"), Local.RunId, FString(TEXT("local")));
    TestEqual(TEXT("Local label uses configured version"), Local.FormatLabel(),
              TEXT("v") + ProjectVersion + TEXT(" / Build local"));

    const FString Path = Directory / TEXT("build-info.json");
    const FString Stamp = TEXT("{\"version\":\"0.3.0-rc.1\",\"buildNumber\":\"12\","
                               "\"buildVersion\":\"0.3.0-rc.1+build.12\",\"runId\":\"identity-fixture\","
                               "\"configuration\":\"Shipping\"}");
    if (!TestTrue(TEXT("Write valid stamp"), FFileHelper::SaveStringToFile(Stamp, *Path)))
        return false;
    const auto Build = FSABuildInfo::Load(Path);
    TestEqual(TEXT("Load stamped semantic version"), Build.Version, FString(TEXT("0.3.0-rc.1")));
    TestEqual(TEXT("Load stamped build number"), Build.BuildNumber, FString(TEXT("12")));
    TestEqual(TEXT("Load full build version"), Build.BuildVersion, FString(TEXT("0.3.0-rc.1+build.12")));
    TestEqual(TEXT("Load stamped run ID"), Build.RunId, FString(TEXT("identity-fixture")));
    TestEqual(TEXT("Load stamped configuration"), Build.Configuration, FString(TEXT("Shipping")));
    TestEqual(TEXT("Intro label identifies stamped build"), Build.FormatLabel(), FString(TEXT("v0.3.0-rc.1 / Build 12")));

    const TArray<TPair<FString, FString>> Invalid = {
        {TEXT("Malformed"), TEXT("{\"version\":")},
        {TEXT("Incomplete"), TEXT("{\"version\":\"9.9.9\",\"buildNumber\":\"999\"}")},
        {TEXT("Wrong type"), Stamp.Replace(TEXT("\"Shipping\""), TEXT("42"))},
        {TEXT("Empty field"), Stamp.Replace(TEXT("\"12\""), TEXT("\"\""))}};
    for (const auto &Fixture : Invalid)
    {
        if (!TestTrue(Fixture.Key + TEXT(" stamp written"), FFileHelper::SaveStringToFile(Fixture.Value, *Path)))
            return false;
        const auto Fallback = FSABuildInfo::Load(Path);
        TestEqual(Fixture.Key + TEXT(" stamp uses complete local identity"), Fallback.FormatLabel(), Local.FormatLabel());
        TestEqual(Fixture.Key + TEXT(" stamp discards build version"), Fallback.BuildVersion, Local.BuildVersion);
        TestEqual(Fixture.Key + TEXT(" stamp discards run ID"), Fallback.RunId, Local.RunId);
        TestEqual(Fixture.Key + TEXT(" stamp discards configuration"), Fallback.Configuration, Local.Configuration);
    }
    return true;
}
#endif
